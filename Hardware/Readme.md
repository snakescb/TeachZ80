# TeachZ80 Hardware

Currently version 1.2 is the first version released

## Design
Teach Z80 is designed with the free, web-based EasyEDA (Standard) service. The whole project is publicly shared on OSHWLab [here](https://www.google.com](https://oshwlab.com/luethich80/pb-z80)https://oshwlab.com/luethich80/pb-z80). Feel free to access the project for detailled information, or clone the project and change it as you like.
Currently, the BOM includes only parts which are assembled by the JLCPCB assembly service. If a full BOM including additional components to finish the board, please let me know and I will create it.

## Configuration
3 jumpers are available on the board for configuration. As often this is not really required, the pinheaders must not be populated, the board features solder jumpers on the back where the required connections can be very easily closed with a soldering iron.
* Flash Bank 1: When closed, it will set Flash address line 16, so all flash accesses will go to the the second 62kB page of the installed FLASH
* SIO Clock Select: SIO A and B clock signals can be either connected to the boards synthetic clocks, or the CTC outputs (CTC 1 for SIO A, CTC 2 for SIO B, for compatibility with Johns code)
* CTC Clock Enable: When populated, CTC 1 and/or CTC 2 TRG signals are connected with the synthetic clocks as on the Z80 Retro. If open, these signals are available on the extension connector for other purposes.

## Get your own TeachZ80
* Empty or SMD assembled PCB's can be ordered from JLCPCB directly through the shared EasyEDA project, with no additional work. They produce as low as 5 PCB's or 2 assembled boards (+3 empty PCB's) minimum order quantity. In my current V1.2 order I purchased 5 SMD assembled boards, at around USD 200 including shipping. There is some tax to my country on top, in the end a single (DMS) assembled board is around USD 50. It's not really cheap, but quality is absolutely on spot.
* I am happy to sell my assembled surplus boards ato no additional cost on top of what I payed to JLCPCB, plus shipping. So far all my assembled boards worked with no issues, but of course there is no guarantee.
* Kits: In case somebody is interested in a kit of the PCB including SMD parts assembled, and a bag of all additional parts required to finish the build, I am happy to do so. I don't have currently a price, but estimate around USD 150 including all passive components, headers, sockets and logic chips (incl. CPU, CTC, SIO, Flash, RAM...)
