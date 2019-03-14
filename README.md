# logue-sdk 

This repository contains all the files and tools needed to build custom oscillators and effects for the [prologue](https://www.korg.com/products/synthesizers/prologue) and [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd) synthesizers.

#### Overall Structure:
* [platform/prologue/](platform/prologue/) : prologue specific files, templates and demo projects.
* [platform/minilogue-xd/](platform/minilogue-xd/) : minilogue xd specific files, templates and demo projects.
* [platform/ext/](platform/ext/) : External dependencies and submodules.
* [tools/](tools/) : Installation location and documentation for tools required to build projects and manipulate built products.
* [devboards/](devboards/) : Information and files related to limited edition development boards.

## Installation

#### OSX & Linux
1. Clone the project with ```git clone https://github.com/korginc/logue-sdk.git```
2. Go into project directory and run ```git submodule update --init``` (installs CMSIS dependency)
3. Install the necessary tools (see [tools/](tools/))

To see if your the SDK has been set-up properly:
- Go to [platform/minilogue-xd/demos/waves/](platform/minilogue-xd/demos/waves/)
- run ```make```

## Sharing your Oscillators/Effects with us

To show us your work please reach out to *logue-sdk@korg.co.jp*.

## Support

The SDK is provided as-is, no technical support will be provided by KORG.

<!-- ## Troubleshooting -->





