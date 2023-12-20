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

![newwmgr](https://github.com/Elektraglide/tek4404/assets/41291895/04b68031-53af-43e8-b41e-e3e3c9e54f39)

And running with MagnoliaFixed6 font.

![wmgr6](https://github.com/Elektraglide/tek4404/assets/41291895/3cc6cbdd-c736-4d4a-ae1a-96cf36e254ef)

