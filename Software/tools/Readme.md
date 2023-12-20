# Software Tools

2 python tools are currently available. Mainly the flashloader is of interest tough

## flashLoader.py

### Purpose
* Reads the provided .bin file and converts it into several Intel HEX records
* Checks on which USB port a TeachZ80 (in Flash Mode) is connected
* When the board is found, sends the HEX records to the Board which then
  * Erases the flash
  * Burns the received data to the flash
  * After the last record, resets the Z80 and gives back control over the bus, which starts the new software
 
 ### Requirements
 * python3 installed on the system. [Python](https://www.python.org/)
 * pySerial installed on the system. ``` pip3 install pySerial ``` [pySerial](https://pypi.org/project/pyserial/)

### Usage
```
python3 flashLoader.py <binfile.bin>
```
```
koebi@rpi4b:~/TeachZ80/Software/Z80/test$ python3 ../../tools/flashLoader.py serial-polled.bin

FlashLoader Script Version 1.0

TeachZ80 fount on /dev/ttyUSB0
488 data bytes available in 'serial-polled.bin'
32 HEX records created

Download record    1 of   32 -   3% - [=                   ] - :100000003E05D3103E00D32021000011000001E87E - CONFIRMED
Download record    2 of   32 -   6% - [=                   ] - :1000100001EDB0DB70310000CD2701CD2201215C64 - CONFIRMED
Download record    3 of   32 -   9% - [==                  ] - :1000200000CDCE0121000001E8011E01CD5B0121C0 - CONFIRMED
Download record    4 of   32 -  12% - [==                  ] - :10003000B800CDCE01CD450021E500CDCE01CD5299 - CONFIRMED
Download record    5 of   32 -  16% - [===                 ] - :100040000076C341000E20CD41010C3E7FB9C2476E - CONFIRMED
Download record    6 of   32 -  19% - [====                ] - :1000500000C9CD53014FCD4101C352000D0A0D0A15 - CONFIRMED
Download record    7 of   32 -  22% - [====                ] - :1000600048656C6C6F20576F726C64202D2054654E - CONFIRMED
Download record    8 of   32 -  25% - [=====               ] - :100070006163685A383020737065616B696E672000 - CONFIRMED
Download record    9 of   32 -  28% - [======              ] - :100080002D2057656C6C20646F6E65210D0A0D0A7A - CONFIRMED
Download record   10 of   32 -  31% - [======              ] - :10009000546869732069732074686520636F6465B0 - CONFIRMED
Download record   11 of   32 -  34% - [=======             ] - :1000A00020492063757272656E746C79206578657D - CONFIRMED
Download record   12 of   32 -  38% - [========            ] - :1000B000637574653A0D0A000D0A416E6420686527 - CONFIRMED
Download record   13 of   32 -  41% - [========            ] - :1000C000726520746865207072696E7461626C6517 - CONFIRMED
Download record   14 of   32 -  44% - [=========           ] - :1000D00020415343494920636861726163746572CA - CONFIRMED
Download record   15 of   32 -  47% - [=========           ] - :1000E000733A0D0A000D0A0D0A492077696C6C20DD - CONFIRMED
Download record   16 of   32 -  50% - [==========          ] - :1000F0006E6F77206563686F2077686174657665D9 - CONFIRMED
Download record   17 of   32 -  53% - [===========         ] - :100100007220796F7520747970653A0D0A00DB32C0 - CONFIRMED
Download record   18 of   32 -  56% - [===========         ] - :10011000E604C9DB33E604C9DB32E601C9DB33E6BA - CONFIRMED
Download record   19 of   32 -  59% - [============        ] - :1001200001C90E33C329010E322131010607EDB397 - CONFIRMED
Download record   20 of   32 -  62% - [============        ] - :10013000C918044403C10568CD130128FB79D331E4 - CONFIRMED
Download record   21 of   32 -  66% - [=============       ] - :10014000C9CD0E0128FB79D330C9CD1D0128FB3A5A - CONFIRMED
Download record   22 of   32 -  69% - [==============      ] - :100150003100C9CD180128FBDB30C9F5D5E5C5C391 - CONFIRMED
Download record   23 of   32 -  72% - [==============      ] - :100160007F017BB7280E7DE60F2811FE0820050EC3 - CONFIRMED
Download record   24 of   32 -  75% - [===============     ] - :1001700020CD41010E20CD4101C39101CDE1017C93 - CONFIRMED
Download record   25 of   32 -  78% - [================    ] - :10018000CDA5017DCDA5010E3ACD41010E20CD4179 - CONFIRMED
Download record   26 of   32 -  81% - [================    ] - :10019000017ECDA50123C10BC578B120C5CDE101FC - CONFIRMED
Download record   27 of   32 -  84% - [=================   ] - :1001A000C1E1D1F1C9F5CB3FCB3FCB3FCB3FCDBA7E - CONFIRMED
Download record   28 of   32 -  88% - [==================  ] - :1001B00001F1F5E60FCDBA01F1C9C630FE3AFAC336 - CONFIRMED
Download record   29 of   32 -  91% - [==================  ] - :1001C00001C6074FC34101E3CDD40123E3C9E5CD07 - CONFIRMED
Download record   30 of   32 -  94% - [=================== ] - :1001D000D401E1C97EB728084FCD410123C3D40122 - CONFIRMED
Download record   31 of   32 -  97% - [=================== ] - :0801E000C9CDC7010D0A00C9D9 - CONFIRMED
Download record   32 of   32 - 100% - [====================] - :00000001FF - CONFIRMED

DOWNLOAD COMPLETED SUCCESSFULLY
```

