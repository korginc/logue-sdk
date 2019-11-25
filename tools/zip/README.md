## Zip Utility

The `zip` (Info-Zip) command line utility version 3.0 is required to package SDK projects.

### OSX

An appropriate zip utility is available by default so no extra steps are necessary.

### Linux

If not already installed, install the appropriate package from your distribution.

### Windows

#### MSYS2

MSYS2 provides Info-Zip 3.0 as the `zip-3.0-1` package.

Install it by running `pacman -S zip`.

_Note: MSYS2 installers for Windows can be found at [https://www.msys2.org/](https://www.msys2.org/)_

#### Manual Procedure: GnuWin

A Windows build of Info-Zip 3.0 is also available from [GnuWin's sourceforge page](https://sourceforge.net/projects/gnuwin32/files/zip/3.0/)

If using this option, install the required files in this directory following this structure:

```
zip/
   |
   +-- bin/zip.exe
```
_e.g.: `zip.exe` should be located in a subdirectory named `bin`._
