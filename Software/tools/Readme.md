# Software Tools

2 python tools are currently available. Mainly the flashloader is of interest tough

## flashLoader.py
The flashloader is doing the following tasks when executed:
* Reads the provided .bin file and converts it into several Intel HEX records
* Checks on which USB port a TeachZ80 (in Flash Mode) is connected
* When the board is found, sends the HEX records to the Board which then
  * Erases the flash
  * Burns the received data to the flash
  * After the last record, resets the Z80 and gives back control over the bus, which starts the new software
