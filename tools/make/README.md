## GNU Make

GNU Make version 3.8.1 or higher is required to build SDK projects.

### OSX

The built-in GNU Make of OSX should be sufficient to build SDK projects so no extra steps are necessary. 
Alternatively, newer versions of GNU Make can be obtained via package distribution systems such as Homebrew and MacPorts.

### Linux

An appropriate GNU Make version is likely already installed. If not, install the appropriate GNU Make package from your distribution.

### Windows

#### MSYS2

MSYS2 provides recent GNU Make packages appropriate for building logue SDK projects.

Install GNU Make by running: `pacman -S make`.

_Note: MSYS2 installers for Windows can be found at [https://www.msys2.org/](https://www.msys2.org/)_

#### Manaual Procedure: GnuWin

A Windows build of GNU Make (albeit a bit old) is also available from [GnuWin's sourceforge page](https://sourceforge.net/projects/gnuwin32/files/make/3.81/).
Once installed you will have to add it manually to your environment's path.


