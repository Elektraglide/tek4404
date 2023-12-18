# tek4404
various C projects for Tektronix4404

Generally uses #ifdef __clang__ for compiling on modern Un*x 

- **ph.h**          - reverse engineered executable header structure<br>
- **strings.c**     - if executable, skips to data segment<br>
- **dumpscreen.c**  - seeks into /dev/pmem to video memory and writes a 1-bit BMP file<br>
- **telnetd.c**  - listens on port 23 and creates a pty running argv[1]<br>
- **ifdump.c**  - uses diddle() to extract MAC address, assigned IP and stats<br>
- **dhcp.c**  - DHCP client; needs router address for Uniflex since we cant do SO_BROADCAST <br>
- **ash.c**  - replacement minimal shell with history, autocomplete, jobs & various builtins that runs in 36k not 350k <br>

- **uniflexshim.h**  - working but incomplete Uniflex emulation to allow compiling these on Darwin<br>

- **tek_graphics.c** - working but incomplete implementation of Tek4404 graphics libs using SDL 
- **wmgr** implements a simple desktop with windows running a shell via a pseudo terminal.  Implements vt100.

![wmgr](https://github.com/Elektraglide/tek4404/assets/41291895/c57c3a41-32e3-4e40-88c1-b596f6ee6bb7)
