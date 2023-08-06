# tek4404
various C projects for Tektronix4404

- **ph.h**          - reverse engineered executable header structure<br>
- **strings.c**     - if executable, skips to data segment<br>
- **dumpscreen.c**  - seeks into /dev/pmem to video memory and writes a 1-bit BMP file<br>
- **telnetd.c**  - listens on port 8023 and creates a pty running argv[1]<br>
- **uniflexshim.h**  - incomplete Uniflex emulation to allow compiling these on Darwin<br>

- **tek_graphics.c** - incomplete implementation of Tek4404 graphics libs using SDL 
- **wmgr** implements a simple desktop with windows running a shell via a pseudo terminal.  Implements vt100.

![alt text](https://github.com/Elektraglide/tek4404/blob/main/wmgr/tek4404_wmgr.png?raw=true)

