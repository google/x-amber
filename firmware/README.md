# NOTICE
* The materials covered by the associated license relate to a general purpose product and its underlying designs, schematics, and hardware and software code (collectively “Contributions”), none of which have been evaluated, cleared or approved for any medical or other purpose by and may not meet applicable safety or other legal or regulatory requirements of any governmental agency, including but not limited to the United States Food and Drug Administration (“FDA”), or equivalent regulatory bodies outside of the US.
* Any use of the term “EEG” or “electroencephalogram” contained in the Contribution is for informational purposes only and does not label, represent or otherwise indicate the Contribution is intended to be used or has been reviewed or approved as a medical device by the US FDA or equivalent regulatory bodies outside of the US.
* Any article, material or other publication by any person or entity referencing the use or operation of the Contribution for a medical or other any other purpose is for informational purposes only; does not directly or indirectly represent or infer any intended use in the diagnosis of disease or other conditions, or in the cure, mitigation, treatment, or prevention of disease, in people or animals; and does not label, represent or otherwise indicate the Contribution is intended to be used or has been reviewed or approved as a medical device by the US FDA or equivalent regulatory bodies outside of the US.
* While the Contributions may have potential medical applications, many countries, including the US, require regulatory approvals prior to using any device in the diagnosis of disease or other conditions, or in the cure, mitigation, treatment, or prevention of disease, in people or animals.
* It is the user’s responsibility to secure all necessary regulatory approvals and meeting all applicable government safety and environmental standards associated with use of the Contributions.

# Instructions
#### Get Source

Directory: `firmware/source/Luchador Serial Firmware KL27/`


#### Install Tools

Download [KDS](https://www.nxp.com/design/designs/design-studio-integrated-development-environment-ide:KDS_IDE?tab=Design_Tools_Tab#nogo)

Linux: `dpkg -i kinetis-design-studio\_3.2.0-1\_amd64.deb`

IDE installed in `/opt/Freescale/KDS\_v3/eclipse/kinetis-design-studio`


#### Add Project

In Project Explorer panel, right-click > Import project

Select an import source > General > Existing Projects into Workspace

Select root directory: Choose `path/to/firmware/source/Luchador Serial Firmware KL27`
*   This should find a project named “Luchador Serial Firmware KL27”

Click Finish


#### Build

Project > Build All


#### Flashing

1. Run > Flash From File

1. Right click GDB PEMicro Interface Debugging > New

1. Connect debugger ([PEMicro MultiLink Universal](http://www.pemicro.com/products/product_viewDetails.cfm?product_id=15320168&productTab=1)) and Luchador board (make sure board is powered up by USB)

1. Install USB Multilink 2.0 driver.

1. Go to Debugger tab, make sure interface and port are selected

1. Select Target Device -> KL27Z256M4

1. Apply

1. Click Flash

1. Disconnect board from debugger and power


#### Resources
See Luchador3.0HWNotes.md.
