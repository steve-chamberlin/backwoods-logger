Backwoods Logger Instructions
October 11, 2011
Steve Chamberlin
steve@bigmessowires.com
www.bigmessowires.com

  

BUILDING YOUR BACKWOODS LOGGER

1. Buy the required parts. The total cost should be around $30-$40 USD. The parts required depend on which version of the logger you want to 
build. See partlist.txt for the list of required parts. 

2. Make or buy the circuit board (PCB), or prepare a breadboard. Visit www.bigmessowires.com and ask if I have any extra boards available. 
If not, send the included Gerber files to a PCB prototyping service, and they can manufacture a set of boards for about $25. If you are 
building the logger on a breadboard instead of a PCB, follow the instructions for the Logger Classic.

3. Assemble the board. The Logger Classic uses all through-hole parts, and can be assembled in an hour or two. The Logger Mini uses all 
surface-mount parts, and is more challenging to assemble, but can still be assembled by hand (I did it!). 

  Logger Classic: 
    The BMP085 breakout board comes without the header attached, so you must solder your own header to it. Solder male header to the *same* 
    side as the BMP085 chip. When you later mount the breakout board onto the socket on the main board, it should be upside down, with the 
    BMP085 on the underside, hidden from view.

  Logger Mini: 
    The SQ-SEN-200 vibration sensor can be omitted. This part is only available directly from Signal Quest, with a minimum order quanity of 
    20. If omitted, the trip timer feature will not work.

    Older versions of the schematic and board layout show an external EEPROM chip. This was originally intended to store longer-term or 
    higher-resolution altitude/temperature data, but was never implemented.

    I recommend you don't solder the battery holder into place until everything else is built and tested. Use alligator clips or pieces of 
    temporary wire to power the board before the battery holder is in place. Once the battery holder is in, it becomes very difficult to 
    probe or fix solder joints for the adjacent components.

    There may be a clearance problem between the battery holder and a few of the surface-mount components closest to it. If necessary, cut 
    away a small portion of the battery holder plastic with a knife to make it fit. Do not cut away any more plastic than necessary, or the 
    battery holder may bend when a battery is inserted.

    The bare BMP085 chip is difficult to hand-solder, because the pins are on its bottom, inaccessable with a soldering iron. To solder it, 
    first tin all 8 pads on the PCB with a small amount of solder. Make sure the heights of the solder blobs are as even as possible from pad 
    to pad. Next, apply flux to the BMP085 pins, and place it in position on top of the tinned pads. While pushing down on the chip from above 
    with a pick, heat the exposed portion of each PCB pad, one at a time, until the solder melts. The heat will be conducted along the pad, 
    under the chip, and the molten solder will bond to the hidden BMP085 pin. You may need to repeat this procedure several times to get a good 
    solder connection.  

4. Get an AVR programmer. I use the AVRISP mkII, which is $35 from Digi-Key. The $22 USBtinyISP AVR programmer is another popular choice: 
see http://www.adafruit.com/products/46

5. Install the battery.

6. Connect the programmer to the board's ISP header. Run AVR Studio, avrdude, or other AVR programming software. Confirm that the board is 
working electrically, but connecting to the AVR and reading the device ID.

7. Program the AVR with the Backwoods Logger firmware. Use the programming software to load the .hex file into the AVR's Flash memory. 
You can use the precompiled hikea.hex file, or compile the included C source files and make your own .hex file.

8. Set the EESAVE fuse on to prevent your altitude/temperature data from being erased every time you reprogram the AVR. 



MODIFYING THE LOGGER

You are encouraged to experiment with changes to the Backwoods Logger software and hardware. Development is an open source, open hardware
project that welcomes everyone's involvement. The project's home page is http://code.google.com/p/backwoods-logger/.

Some ideas for further development:

- Add more detailed analysis tools to the software. Rate of ascent graphs, day by day statistics comparisons, user-defined graph 
timescales, etc.

- Add a way to download the stored sample data to your PC.

- Preload the elevation profile of your expected route onto the logger before starting a trip, then compare measured altitude samples with 
the preloaded profile to determine where you are.

- Make a version of the Logger Classic that uses the higher-resolution OLED found on the Mini. 



BUILDING THE SOFTWARE

The Backwoods Logger source code was built with AVR Studio 5, and the hikea.avrgccproj file will only work with AVR Studio 5. If you are 
using a different version of AVR Studio, or a different compiler entirely, you will need to create your own project file or makefile with 
the following settings:

Logger Classic
  define these symbols: LOGGER_CLASSIC, NOKIA_LCD, F_CPU=1000000
  include these libraries: libm.a
  change these fuses: EESAVE on. Fuse settings are low: 0x62, high: 0xD1, extended: 0xFF

Logger Mini
  define these symbols: LOGGER_MINI, SSD1306_LCD, F_CPU=8000000, SHAKE_SENSOR (optional)
  include these libraries: libm.a
  change these fuses: EESAVE on, CKDIV8 off. Fuse settings are low: 0xE2, high: 0xD1, extended: 0xFF

Logger on a breadboard:
  follow the instructions for Logger Classic.

If you want to use the original software as-is without compiling, you can also use the precompiled hikea.hex files in the Classic or Mini 
subdirectories.



USING THE SCHEMATIC AND BOARD LAYOUT

The Eagle .sch and .brd files can be found in the Classic and Mini subdirectories. In addition to the standard Eagle libraries, you will 
also need the SparkFun library, Adafruit library, and Big Mess o Wires library. The Sparkfun and Adafruit libraries can be downloaded from 
their respective web sites. 

https://github.com/sparkfun/SparkFun-Eagle-Library
http://www.ladyada.net/library/pcb/eaglelibrary.html

The BMOW library is included here, as big-mess-o-wires.lbr.

PIEZO BUZZER NOTE: The Logger Classic uses a Murata PKM13EPYH4002 piezo buzzer, which has a 5mm pin spacing. I was foolish and added a new 
5mm piezo footprint to the Sparkfun library. Your version of the Sparkfun library will not have this footprint, so you will get an error 
when opening the schematic of layout file. If you wish to use this same buzzer, you can find the appropriate part in the big-mess-o-wires 
library, named PIEZO.



LICENSING

The Backwoods Logger software is offered under the MIT license, whose terms appear in the source code files. The Backwoods Logger
schematics, board layout files, Gerber files, and other content files are offered under the Creative Commons Attribution 3.0 license
(CC BY 3.0), whose terms can be viewed at http://creativecommons.org/licenses/by/3.0/.  
