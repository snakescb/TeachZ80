# TeachZ80
Retro Z80 single board computer base on John Winans original design: [Z80-Retro!](https://github.com/Z80-Retro) 

I fell in love (again) when I stumbled over Johns work, and his amazing YouTube [Playlist](https://www.youtube.com/playlist?list=PL3by7evD3F51Cf9QnsAEdgSQ4cz7HQZX5) where he not only explains the hardware in detail, but provides excellent educational content about the hardware, software, assembler, CP/M, compiling and everything related to revive this technology from the late 70's. From there, I knew I want to design and build my own boards, so here it is: TeachZ80. 

![tz80](https://github.com/snakescb/TeachZ80/assets/10495848/9f057115-1c9f-49a7-9c8c-1d03e33ec62e)


TeachZ80 is fully software compatible with Johns Z80-Retro board. It is 95% the same design, but includes some additional (modern) hardware components to make the system even more accessible and easier to use. Beside the modern SMD parts, the board preserves some of the retro look-and-feel, and can be easily hand soldered. 

More information about the hardware [here](https://github.com/snakescb/TeachZ80/tree/main/Hardware)

## Changes and Features versus the original Z80-Retro!

* Additional components and tools
  * SI5351A clock Generator. Generates 3 adjustable clocks for the, 1 for the Z80 system clock and one for each SIO channel
  * STM32F722 support Processor
    * Downloads user compiled assemblies into the FLASH directly through the USB port
    * Provides a command line interface for easy access to core functions:
      * Changing System and SIO clock frequencies
      * Erase the FLASH
      * Dump the FLASH content
      * Load the FLASH with precompiled built-in testprograms
    * Handles Z80 IO-Write-Request to address 0x60 - 0x60F, so the Z80 can access some STM32 functionality as well (like changing clock frequencies or controlling the STM32 LED through IO requests)
    * More potential future features (let's see)
      * Dumping some RAM addresses while Z80 is running
      * Set breakpoints to specific RAM addresses and inform user when Z80 is accessing them
      * Assemble code directly in the STM32, so no exteral tools are required?
      * Reading and formatting the SDCard
      * Generating IO-requests to onboard or addon-boards hardware through the console
  * USB-C connector providing up to 15W input power
  * CP2105 Dual-Channel USB to UART oconverter. This will create 2 virtual serial ports to the USB host:
    *   The first port is conected to the STM32, to load Z80 assemblies into to the FLASH, access the stm32 command line interface or upgrading the stm32 firmware
    *   The second port is connected to SIO A, which is used as the default console output by the Z80 software
  * New GPIO Header on the right for more possible addon boards. The printer header from the Z80-Retro has been removed, but all printer signals still are available on this new header. In total the header provides access to:
    *   10 General Purpose Outputs
    *   9 General Purpose Inputs
    *   The 3 synthetic clock signals
    *   SIO A and B Rx and Tx signals
    *   CTC channel 0, 1 and 2 TD and TRG signals
    *   5V and 3.3V
  *  Additional 74LS244 output latch on IO port 0x50. The lower 4 bits are wired to the user LED's (the higher 4 are used for internal 3.3V-5V level shifting).
  *  4 additional General Purpose User LED's
  *  Flash Bank Select Jumper, so the system can access the lower or upper 64kB page of the installed FLASH, selectable by jumper
  *  2 UART headers, accessing SIO A and B on 5V logic levels (no RS232 level shifter installed)


