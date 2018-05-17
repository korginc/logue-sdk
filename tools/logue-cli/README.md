# *logue-cli* Command Line Utility 

This utility allows to manipulate *.prlgunit* files and communicate with the [prologue synthesizer](http://korg.com/prologue) (or [development board](../../devboards/)).

## Installation

### OSX

Run `./get_logue_cli_osx.sh` to download and unpack *logue-cli* automatically.

### Linux

Currently unsupported. Builds will be provided at a later time.

### Windows

#### Standard Procedure

Download the appropriate version below according to your system.

##### Version 0.01-0b
* 32bit : [prologue-win32-0.01-0b.zip](http://cdn.storage.korg.com/korg_SDK/logue-cli-win32-0.03-0b.zip) (SHA1: cfe6e6e774ab03ffc76e99dcff675feb1ca9db29)
* 64bit : [prologue-win64-0.01-0b.zip](http://cdn.storage.korg.com/korg_SDK/logue-cli-win64-0.03-0b.zip) (SHA1: a4106fa2fdc75d68952056dea0a8f9915bcfdbeb)

#### MSYS

If you have msys installed you can run `./get_logue_cli_msys.sh` to download and unpack *logue-cli* automatically.


## Basic Usage

```
Usage: logue-cli <command> [options]

Commands:

  check: Validate unit packaging.
  probe: Obtain information about a connected device.
   load: Load specified unit onto a connected device.
  clear: Clear unit data from a connected device.

Use -h/--help along with a command to obtain detailed usage information.
```

 

