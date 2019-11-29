# *logue-cli* Command Line Utility 

This utility allows to manipulate unit (*.prlgunit*, *.mnlgxdunit*, *.ntkdigunit*) files and communicate with the [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd) and [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) synthesizers.

## Installation

### OSX

Run `./get_logue_cli_osx.sh` to download and unpack *logue-cli* automatically.

### Linux

Run `./get_logue_cli_linux.sh` to download and unpack *logue-cli* automatically.

### Windows

#### MSYS2

If you have msys installed you can run `./get_logue_cli_msys.sh` to download and unpack *logue-cli* automatically.

_Note: MSYS2 installers for Windows can be found at [https://www.msys2.org/](https://www.msys2.org/)_

#### Manual Procedure

Download the appropriate version below according to your system.

##### Version 0.07-2b
* 32bit : [logue-cli-win32-0.07-2b.zip](http://cdn.storage.korg.com/korg_SDK/logue-cli-win32-0.07-2b.zip) (SHA1: a5bb27d2493728900569881c2a9fe366cce1e943)
* 64bit : [logue-cli-win64-0.07-2b.zip](http://cdn.storage.korg.com/korg_SDK/logue-cli-win64-0.07-2b.zip) (SHA1: 3ee94cebce383fb1319425aaa7abf4b30b1c1269)


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

### Examples

* List available MIDI ports:
```
$ ./logue-cli probe -l
```

* Specify explit MIDI port numbers (i.e.: input: 1, output: 2, following indexes obtained from -l):
```
$ ./logue-cli probe -i 1 -o 2
```

* List oscillators currently loaded on connected device:
```
$ ./logue-cli probe -m osc
```

* Load a unit file to a connected compatible device, and place it in the first available slot:
```
$ ./logue-cli load -u my_oscillator.mnlgxdunit 
```

* Load a unit file to a connected compatible device, and place it in a specific slot, overwriting previous content (i.e.: slot 2):
```
$ ./logue-cli load -u my_oscillator.mnlgxdunit -s 2
```

### Troubleshooting

#### Device's MIDI ports are not auto detected

The MIDI ports to use for the SYSEX communication can be specified via command line options, but care must be taken to choose the right ports.
In the case of prologue and minilogue xd, the ports ending with `KBD/KNOB` and `SOUND` must be selected. The other ports refer to the legacy MIDI connectors on the back of the device.

