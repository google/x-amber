# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import serial
import sys
import time
import numpy
import threading
import signal

#	Bootloader baud rate
BTBAUD=19200	

VSYS_MAX=5.5
VSYS_MIN=4.5

V33_MAX=3.4
V33_MIN=3.2

V25P_MAX=2.6
V25P_MIN=2.4

V25N_MAX=-2.4
V25N_MIN=-2.6

AMP_HIGH_MAX=3200
AMP_HIGH_MIN=1900

AMP_LOW_MAX=-3900
AMP_LOW_MIN=-5600

MAX_EDGE_OFFSET=3

FREQUENCY_MAX=5.001
FREQUENCY_MIN=4.998


memoryMap=numpy.full(256*1024,0xff)
startExecAddressString=""
cliReceivedEvent=threading.Event()
dataReceivedEvent=threading.Event()
cliText=""
dataText=""
captureData=False
dataStr=[]
logs=[]

def SignalExit(sig, frame):
	global stopSerialThread

	print("Stopping serial thread")
	stopSerialThread=True
	time.sleep(0.5)
	print("Exitting")
	exit()

signal.signal(signal.SIGINT, SignalExit)


#	Enumerates system serial ports
def GetSerialPorts():     
	if sys.platform.startswith('win'):
		ports = ['COM%s' % (i + 1) for i in range(256)]
	elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
		# this excludes your current terminal "/dev/tty"
		ports = glob.glob('/dev/tty[A-Za-z]*')
	elif sys.platform.startswith('darwin'):
		ports = glob.glob('/dev/tty.*')
	else:
		raise EnvironmentError('Unsupported platform')

	result = []
	for port in ports:
		try:
			s = serial.Serial(port)
			s.close()
			result.append(port)
		except (OSError, serial.SerialException):
			pass
	return result

#	Sends a list of bytes to the serial port
def SerialSendBytes(serialPort, data):
	for x in data:
		serialPort.write(chr(int(x)))

#	Reads a list of bytes from the serial port
def SerialReadBytes(serialPort, length):
	data=[]
	sdata=serialPort.read(length)	 	
	for i in sdata:
		data.append(ord(i))
	return data

#	Waits for a USB com port to be detected
def WaitForUsbDetect():	   
	ports1=GetSerialPorts()
	while True:			
		ports2=GetSerialPorts()
		diff=list(set(set(ports2)-set(ports1)))
		print ".",
		if len(diff)!=0:
			Log("\n\rPort {} detected".format(diff[0]))
			time.sleep(1.0)
			return diff[0]
		ports1=ports2
		time.sleep(1.0)
		

#	Attempts to connect to the KL27 factory bootloader
#	Refer to the reference manual to bootloader detection
def ConnectToBootloader(serialPort): 
	serialPort.flushInput()
	serialPort.flushOutput()
	SerialSendBytes(serialPort, [0x5a, 0xa6])
	data=SerialReadBytes(serialPort,10)
	Log(data)
	if len(data)!=10:return False
	if data[0:2]==[0x5a,0xa7]:return True
	return False

#	Clears the memory structure to 0xff
def ClearMemoryMap():
	memoryMap=numpy.full(256*1024,0xff)

#	Loads and parses a hex file into the memory structure
def LoadHexFile(file):
	global startExecAddressString
	offset=0
	bytesLoaded=0
	firstAddress=-1
	lastAddress=-1
	with open(file) as hexfile:
		for line in hexfile:
			
			if line[0]!=":":raise AssertionError("Error in hex file:"+line)
			#	Parse bytecount
			bytecount=int(line[1:3],16)
			#	Parse address
			address=int(line[3:7],16)+offset
			if firstAddress==-1:
				firstAddress=address
			rtype=int(line[7:9],16)
			#	Process data type
			if rtype==0:
				for x in range(bytecount):
					data=int(line[9+(2*x):11+(2*x)], 16)
					memoryMap[address]=data
					lastAddress=address
					bytesLoaded+=1
					address+=1
			#	Process extended offset
			elif rtype==2:
				offset=int(line[9:13],16)<<4
			#	Process the start pointer (not critical to ARM processor)
			elif rtype==3:
				startExecAddressString=line

	return lastAddress-firstAddress+1

#	Creates a generic bootloader packet
def CreateBootloaderPacket(packetType, payload):
	packet=[0x5a, packetType, len(payload) & 0xff, (len(payload) >> 8) & 0xff]
	packet.extend(payload)
	crc=0
	for b in packet:
		crc=(crc^(b<<8))%2**32
		for i in range(8):
			temp=crc<<1
			if crc & 0x8000:
				temp=(temp^0x1021)%2**32
			crc=temp
	crcH=int((crc>>8)&0xff)
	crcL=int(crc & 0xff)
	packet[4:4]=[crcL, crcH]
	return packet

#	Creates a bootloader write memory packet subtype
def CreateWriteMemoryCommand(startAddress, length):
	#	Conforms to the Command packet type described in the reference manual
	packet=[0x04,0x00,0x00,0x02,startAddress&0xff,(startAddress>>8)&0xff, (startAddress>>16)& 0xff, (startAddress>>24) & 0xff,length&0xff, (length>>8)&0xff,(length>>16)&0xff, (length>>24)& 0xff]
	return packet

#	Programs the device
def BootloaderProgramDevice(serialPort, addressStart, length):
	#	Create the packet to signal bootloader to write memory
	packet=CreateBootloaderPacket(0xa4, CreateWriteMemoryCommand(addressStart, length))
	SerialSendBytes(serialPort, packet)
	#	Check of ACK
	res=SerialReadBytes(serialPort,2)
	if res!=[0x5a, 0xa1]:raise AssertionError("ACK not received from bootloader")
	#	Check generic response
	res=SerialReadBytes(serialPort, 18)
	if len(res)!=18:raise AssertionError("Incorrect generic response received from command packet")
	#	Send ACK to response
	SerialSendBytes(serialPort, [0x5a, 0xa1])
	#	Start creating data packets
	address=addressStart
	count=0
	lastPrinted=0
	while count<length:
		data=[]
		for x in range(16):
			data.append(memoryMap[address])
			address+=1
			count+=1
			if count==length:break
		success=False
		for retry in range(5):
			packet=CreateBootloaderPacket(0xa5,data)
			SerialSendBytes(serialPort,packet)
			#	Wait for ack
			res=SerialReadBytes(serialPort,2)
			if res!=[0x5a, 0xa1]:
				#raise AssertionError("ACK not received after data byte {}".format(count))
				Log("Resending packet {}".format(count))
				continue
			else:
				success=True
				break
		if not success:	raise AssertionError("ACK not received after data byte {}".format(count)) 
		percent=count*100/length
		if percent%10==0 and lastPrinted!=percent:
			Log("{} %".format(count*100/length))
			lastPrinted=percent

	#	Get final response
	res=SerialReadBytes(serialPort,18)
	if len(res)!=18:
		Log(res)
		raise AssertionError("Incorrect generic response for final packet len={}".format(len(res)))
	#	Send final ACK
	SerialSendBytes(serialPort, [0x5a, 0xa1])

#	Resets the board
def BootloaderReset(serialPort):
	#	Send reset command
	SerialSendBytes(serialPort, [0x5a, 0xa4,0x04, 0x00, 0x6f, 0x46, 0x0b, 0x00, 0x00, 0x00])
	res=SerialReadBytes(serialPort,2)
	#	Check ACK
	if res!=[0x5a, 0xa1]:raise AssertionError("ACK not received after reset request")
	res=SerialReadBytes(serialPort, 18)
	if len(res)!=18:raise AssertionError("Generic response length error after reset request")
	#	Send final ACK
	SerialSendBytes(serialPort, [0x5a, 0xa1])

def BootloaderEraseModule(serialPort):
	SerialSendBytes(serialPort, [0x5a, 0xa4, 0x04, 0x00, 0xc4, 0x2e, 0x01, 0x00, 0x00, 0x00])
	res=SerialReadBytes(serialPort,2)
	#	Check ACK
	if res!=[0x5a, 0xa1]:raise AssertionError("ACK not received after reset request")
	res=SerialReadBytes(serialPort, 18)
	if len(res)!=18:raise AssertionError("Generic response length error after reset request")
	#	Send final ACK
	SerialSendBytes(serialPort, [0x5a, 0xa1])



#	Main thread used to receive serial data from a working Amber board
def SerialThread(serialPort):
	global stopSerialThread
	global cliText
	global dataText
	global cliReceivedEvent
	global dataReceivedEvent
	global captureData
	global dataStr
	
	dataline=""
	parseLine=""
	while not stopSerialThread:
		#	Block until data comes in
		dataline=serialPort.read(1)
		btr=serialPort.inWaiting()
		dataline+=serialPort.read(btr)
		#	Parse the received data
		for data in dataline:
			#	If return received, check if the line contains CLI or DATA and fire off the event
			if data=="\r":
				if 	"CLI:" in parseLine:
					cliText=parseLine
					cliReceivedEvent.set()
				elif "DATA:" in parseLine:
					dataText=parseLine
					dataReceivedEvent.set()
					if captureData:dataStr.append(dataText)						
				parseLine=""
			else:
				parseLine+=data


#	Sends a command to the Amber CLI
def SendCliCommand(serialPort, command):
	global cliReceivedEvent

	cliReceivedEvent.clear()
	serialPort.write(command+"\r")


#	Checks power supply voltages
def CheckVoltages(serialPort):
	global cliReceivedEvent
	global cliText

	#	Send 'diag' command
	SendCliCommand(serialPort,"diag")
	if cliReceivedEvent.wait(1000)==False:raise AssertionError("Timed out waiting for response to diag command")
	cliSplit=cliText.split(':')
	if len(cliSplit)!=2:raise AssertionError("Missing colon in CLI response:"+cliText)
	results=cliSplit[1].split(',')
	if len(results)!=4:raise AssertionError("Wrong value count received:"+cliText)
	args=results[0].split('=')
	vsys=float(args[1])
	args=results[1].split('=')
	v33=float(args[1])
	args=results[2].split('=')
	v25p=float(args[1])
	args=results[3].split('=')
	v25n=float(args[1])
	
	Log("\n\r***VSYS***")
	result=CheckRange(vsys,VSYS_MAX,VSYS_MIN)
	Log("\n\r***+3.3V***")
	result&=CheckRange(v33,V33_MAX,V33_MIN)
	Log("\n\r***+2.5V***")
	result&=CheckRange(v25p,V25P_MAX,V25P_MIN)
	Log("\n\r***-2.5V***")
	result&=CheckRange(v25n, V25N_MAX, V25N_MIN)

	return result


#	Checks that val falls within max and min
def CheckRange(val, max, min):
	result=""
	if min <= val <= max:
		result="PASS"
	else: result="FAIL"
	Log("VAL\t\tMAX\tMIN\tRESULT")
	Log("{}\t{}\t{}\t{}".format(val,max,min,result))
	if result=="PASS":return True
	return False

#	Does a comprehensive test of the ADS1299 converters
def TestAds1299(serialPort):
	global cliReceivedEvent
	global cliText
	global dataReceivedEvent
	global dataText
	global dataStr
	global captureData

	Log("Turning channels off")
	SendCliCommand(serialPort, "chon all,0")
	if cliReceivedEvent.wait(1)==False:raise AssertionError("Timed out waiting for response to chon command")

	Log("Sending test command")
	SendCliCommand(serialPort, "test")
	if cliReceivedEvent.wait(1)==False:raise AssertionError("Timed out waiting for response to test command")
	if "Test mode turned on" not in cliText:raise AssertionError("Incorrect response to test command:"+cliText)
	
	time.sleep(.5)
	Log("Gathering 5 seconds of data") 	
	dataStr=[]
	captureData=True
	time.sleep(5)
	captureData=False
	Log("Acquisition complete, analyzing...")
	Log("Checking for sequential data...")
	if CheckSequential(dataStr):
		Log("PASS")
	else:
		Log("FAIL")
		return False
	Log("Checking Amplitudes...")
	high_max=None
	high_min=None
	low_max=None
	low_min=None
	for channel in range(1,33):
		min,max=GetMinMax(dataStr,channel)
		if max>AMP_HIGH_MAX or max<AMP_HIGH_MIN:
			Log("Channel {} FAILED HIGH VALUE".format(channel))
			CheckRange(max,AMP_HIGH_MAX, AMP_HIGH_MIN)
			return False
		if min<AMP_LOW_MIN or min>AMP_LOW_MAX:
			Log("Channel {} FAILED LOW VALUE".format(channel))
			CheckRange(min, AMP_LOW_MAX, AMP_LOW_MIN)
			return False 
		if high_max==None:high_max=max
		if high_min==None:high_min=max
		if low_max==None:low_max=min
		if low_min==None:low_min=min
		if max>high_max:high_max=max
		if max<high_min:high_min=max
		if min>low_max:low_max=min
		if min<low_min:low_min=min
	Log("{}<HIGH<{}".format(high_min, high_max))
	Log("{}<LOW<{}".format(low_min, low_max))
	Log("PASSED")
	Log("Checking synchronization...")
	if CheckSynchronization(dataStr)==False:
		Log("Synchronization FAILED")
		return False
	Log("PASSED")
	Log("Checking frequency:")
	freq=CheckFrequency(dataStr,1)
	Log("Average frequency={} Hz".format(freq))
	if freq>FREQUENCY_MAX or freq<FREQUENCY_MIN:
		Log("FAILED")
		CheckRange(freq, FREQUENCY_MAX, FREQUENCY_MIN)
		return False
	Log("PASSED")
	return True

#	Measures the average frequency based on detected edges in the data stream
def CheckFrequency(data, offset):
	edges=GetHighEdges(data,offset)
	sum=0
	for e in range(len(edges)-1):
		sum=edges[e+1]-edges[e]
	avg=sum/len(edges)-1
	period=avg*0.004	#	4ms sample rate
	return 1/period

#	Detects rising edges in the data stream
def GetHighEdges(data, offset):
	edges=[]
	lastVal=None
	for i, line in enumerate(data):
		foo=line.split(':')
		if len(foo)!=2:raise AssertionError("Missing colon in data")
		vals=foo[1].split(',')
		val=int(vals[offset])
		if lastVal==None:
			lastVal=val
			continue
		#if val>(AMP_HIGH_MIN/2) and val>(lastVal+500):
		if val>0 and lastVal<0:
			edges.append(i)
		lastVal=val
	return edges

#	Checks that edges are synchronized across all channels
def CheckSynchronization(data):
	edges=[]
	for channel in range(1,33):
		edges.append(GetHighEdges(data,channel))
	for x in range(31):
		chx1=edges[x]
		chx2=edges[x+1]
		for d in range(len(chx1)):
			diff=abs(chx1[d]-chx2[d])
			if diff>MAX_EDGE_OFFSET:		
				Log("Synchro error in channels {} and {}".format(x+1,x+2))
				Log("Delta={}".format(diff))
				return False
	return True

#	Gets the max and min values of the data stream
def GetMinMax(data, offset):
	min=None
	max=None
	for line in data:
		foo=line.split(':')
		if len(foo)!=2:raise AssertionError("Missing colon in data")
		vals=foo[1].split(',')
		val=int(vals[offset])
		if min==None:
			min=val
			max=val
			continue
		if val>max:max=val
		if val<min:min=val
	return (min,max)

#	Checks that the data doesn't have any missing packet IDs
def CheckSequential(data):
	lastId=None
	x=0
	for line in data:
		x+=1
		foo=line.split(':')
		if len(foo)!=2:raise AssertionError("Missing colon in data")
		vals=foo[1].split(',')
		val=int(vals[0])
		if lastId==None:
			lastId=val
			continue
		if lastId+1!=val:
			Log("Missing ID on line {}:".format(x)+line)
			Log("Last ID={}".format(lastId))
			return False
		lastId=val
	return True

#	Checks whether the board is streaming measurement data
def CheckForDataStream(port):
	sp=serial.Serial(port, "921600", timeout=1)
	data=sp.read(500)
	sp.close()
	if "DATA:" not in data:return False
	return True

#	Main function to program and test the board
def ProgramAndTest(filename):
	global cliReceivedEvent
	global dataReceivedEvent
	global stopSerialThread

	sn=""
	program=True
	
	serialTime=time.strftime("%Y%m%d%H%M")
	serialBoard=0
	try:
		if program:
			ClearMemoryMap()
			Log("Loading "+filename)
			btl=LoadHexFile(filename)
			Log("{} bytes loaded".format(btl))
	except Exception as ex:
		Log("ERROR:Loading {}:{}".format(filename,ex.strerror))
		exit()
	while True:
		try:
			LogClear()
			Log("Waiting for USB connect...")
			port=WaitForUsbDetect()	
			Log("Time started:"+time.strftime("%Y-%m-%d %H:%M:%S"))
			reprogram=False
			if program:
				Log("Checking for data stream")
				if CheckForDataStream(port)==True:
					Log("Programmed board detected")
					serialPort=serial.Serial(port, 921600, timeout=0.5)
					stopSerialThread=False
					t1=threading.Thread(target=SerialThread, args=(serialPort,))
					t1.start()
					fail=True
					for retry in range(5):
						SendCliCommand(serialPort,"ver")
						if cliReceivedEvent.wait(1)==False:
							Log("Timed out, retrying...")
							continue
						else:
							fail=False
							break
					if fail:raise AssertionError("Timed out waiting for version")
					Log("Version:"+cliText.replace("CLI:",""))
					fail=True
					for retry in range(5):
						SendCliCommand(serialPort, "ser")
						if cliReceivedEvent.wait(1)==False:
							Log("Timed out, retrying...")
							continue
						else:
							fail=False
							break	 						
					if fail:raise AssertionError("Timed out waiting for serial number")
					Log("Serial number:"+cliText.replace("CLI:Serial:",""))
					sn=cliText.replace("CLI:Serial:","")
					Log("Do you want to reprogram this board?")
					i=raw_input("Enter y to reprogram:")
					if i=='y':
						reprogram=True
						Log("Sending command to enter bootloader")
						SendCliCommand(serialPort,"bootloader")
						stopSerialThread=True
						time.sleep(1)
						serialPort.close()	
						time.sleep(2)
					else:raise AssertionError("Programming CANCELLED")

				Log("Trying to connect to bootloader...")
				  
				connected=False
				for retry in range(25):
					serialPort=serial.Serial(port, BTBAUD, timeout=1) 
					if ConnectToBootloader(serialPort):
						Log("Connected to bootloader")
						connected=True
						break
					serialPort.close()
				if not connected:raise AssertionError("Could not connect to bootloader")
				if reprogram:
					Log("Erasing module...")
					BootloaderEraseModule(serialPort)
					Log("Module erased")
				Log("Programming with "+filename)
				BootloaderProgramDevice(serialPort,0,btl)
				Log("Programming successful")
				Log("Resetting board")
				BootloaderReset(serialPort)
				Log("Reconfiguring Serial Port for 921600 baud")
				serialPort.close()

		
			serialPort=serial.Serial(port, 921600, timeout=1)
			Log("Starting serial thread")
			stopSerialThread=False
			t1=threading.Thread(target=SerialThread, args=(serialPort,))
			t1.start()
			dataReceivedEvent.clear()
			cliReceivedEvent.clear()  
			Log("Waiting for Data to stream")
			if dataReceivedEvent.wait(10)==False:raise AssertionError("Timed out waiting for data stream")
			Log("Getting Versions:")
			fail=True
			for retry in range(5):
				SendCliCommand(serialPort,"ver")
				if cliReceivedEvent.wait(2)==False:
					Log("Timed out, retrying")
					continue
				else:
					fail=False
					break
			if fail:raise AssertionError("Timed out waiting for version response")
			Log(cliText.replace("CLI:",""))
			Log("Checking onboard power supplies:")
			fail=True
			for retry in range(2):
				if not CheckVoltages(serialPort):
					Log("Failed, retrying")
					continue
				else:
					fail=False
					break
			if fail:raise AssertionError("POWER SUPPLY CHECK FAILED")
			Log("Power supplies PASSED")
			time.sleep(3.0)
			Log("Checking ADS1299:")
			fail=True
			for retry in range(2):
				if not TestAds1299(serialPort):
					Log("Failed, retrying")
					continue
				else:
					fail=False
					break
			if fail:raise AssertionError("ADS1299 CHECK FAILED")
			Log("Stopping serial thread")
			 
			Log("**************  ALL TESTS PASSED")
			
			Log("Checking serial number...")
			fail=True
			for retry in range(5):
				SendCliCommand(serialPort, "ser")
				if cliReceivedEvent.wait(1)==False:
					Log("Timed out, retrying")
					continue
				else:
					fail=False
					break
			if fail:raise AssertionError("Timed out waiting for serial number")
			if "NOT_SET" not in cliText:
				Log("Serial number previously set to:"+cliText.replace("CLI:Serial:",""))
				Log("******* Cannot be changed *******")
				append=True
			else:
				append=False
				sn=serialTime+"-"+"{:04}".format(serialBoard)
				Log("Not setting serial number at this time")
				#Log("Setting serial number to "+sn)
				#SendCliCommand(serialPort, "setser "+sn)
				#if cliReceivedEvent.wait(1)==False:raise AssertionError("Timed out waiting to set serial number")
				#if "number set to" not in cliText:
					#raise AssertionError("Serial not set correctly")
				#Log("Serial number set correctly")
				serialBoard+=1
			Log("*************     BOARD PROGRAMMING AND TEST SUCCESSFUL    ***************")
			stopSerialThread=True
			time.sleep(1)
			serialPort.close()	
			Log("\n\rRemove programmed board and insert new board")
			LogSave(sn, append)
		except Exception as ex:
			Log("************ ERROR:"+ex.message)
			stopSerialThread=True
			time.sleep(1)
			serialPort.close()
			Log("Remove board and reconnect")
			LogSave(serialTime,True)

#	Prints the data and saves it to a list to be saved later
def Log(data):
	global logs

	logs.append(str(data))
	print(data)

#	Clears the log
def LogClear():
	global logs

	logs=[]

#	Saves the logged data to a filename
def LogSave(filename, append=False):
	if append:args='a'
	else:args='w'

	filename=filename.replace("\n","")
	filename=filename.replace("\r","")
	with open("./Logs/"+filename+".log", args) as file:
		for line in logs:
			file.write(line+"\n\r")





if __name__ == '__main__':
	
	if len(sys.argv)==1:
		ProgramAndTest("Luchador.hex")
	else:
		ProgramAndTest(sys.argv[1])

