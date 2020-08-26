/*
 * Copyright 2020 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Queue.c
 *
 *  Created on: Dec 3, 2018
 *      Author: nick
 */

#include "Queue.h"
#include "MKL27Z4.h"

QException Queue_Init(Queue_t *queue, uint8_t *buf, int size)
{
	__disable_irq();
	queue->data=buf;
	queue->consumer=0;
	queue->producer=0;
	queue->count=0;
	queue->size=size;
	__enable_irq();
	return QEX_None;
}

QException Queue_Push(Queue_t *queue, uint8_t data)
{
	//__disable_irq();
	if(queue->count==queue->size)
	{
		__enable_irq();
		return QEX_Full;
	}
	queue->data[queue->producer]=data;
	queue->producer=(queue->producer+1) % queue->size;
	queue->count++;
	__enable_irq();
	return QEX_None;
}

QException Queue_Pop(Queue_t *queue, uint8_t *data)
{
	__disable_irq();
	if(queue->count==0)
	{
		__enable_irq();
		return QEX_Empty;
	}
	*data=queue->data[queue->consumer];
	queue->consumer=(queue->consumer+1) % queue->size;
	queue->count--;
	__enable_irq();
	return QEX_None;
}

QException Queue_Get(Queue_t *queue, int pos, uint8_t *data)
{
	__disable_irq();
	if(pos>=queue->count)
	{
		__enable_irq();
		return QEX_IndexOutOfBounds;
	}
	*data=queue->data[queue->consumer+pos];
	__enable_irq();
	return QEX_None;
}

QException Queue_Clear(Queue_t *queue)
{
	__disable_irq();
	queue->consumer=0;
	queue->producer=0;
	queue->count=0;
	__enable_irq();
	return QEX_None;
}

QException Queue_Drop(Queue_t *queue)
{
	__disable_irq();
	if(queue->count==0)
	{
		__enable_irq();
		return QEX_Empty;
	}
	if(queue->producer==0) queue->producer=queue->size-1;
	else queue->producer--;
	queue->count--;
	__enable_irq();
	return QEX_None;
}


