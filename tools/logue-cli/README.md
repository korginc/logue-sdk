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

##### Version 0.06-0b
* 32bit : [logue-cli-win32-0.06-0b.zip](http://cdn.storage.korg.com/korg_SDK/logue-cli-win32-0.06-0b.zip) (SHA1: 5e349adb70b9109a8d174dbb84a39612ad8f30f3)
* 64bit : [logue-cli-win64-0.06-0b.zip](http://cdn.storage.korg.com/korg_SDK/logue-cli-win64-0.06-0b.zip) (SHA1: a6b764a06da87010dbde28b11a2b917cd9fc14c1)

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

### Troubleshooting

#### Device's MIDI ports are not auto detected

The MIDI ports to use for the SYSEX communication can be specified via command line options, but care must be taken to choose the right ports.
In the case of prologue and minilogue xd, the ports ending with `KBD/KNOB` and `SOUND` must be selected. The other ports refer to the legacy MIDI connectors on the back of the device.

