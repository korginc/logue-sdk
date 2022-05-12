# logue-sdk 

[日本語](./README_ja.md)

This repository contains all the files and tools needed to build custom oscillators and effects for the [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) synthesizers, and [drumlogue](https://www.korg.com/products/drums/drumlogue) drum machine.

#### Ready to Use Content

To download ready to use oscillators and effects, refer to the [Unit Index](https://korginc.github.io/logue-sdk/unit-index/) and follow instructions on the developer's website.

#### Compatibility Notes

* prologue: SDK version 1.1-0 is supported by firmware >= v2.00
* minilogue xd: SDK version 1.1-0 is supported by firmware >= 2.00
* Nu:Tekt NTS-1 digital: SDK version 1.1-0 is supported by firmware >= v1.02
* drumlogue: SDK version starts at 2.0-0 and is supported by all currently available firmware versions.

User units built for prologue, minilogue xd and Nu:Tekt NTS-1 digital are binary compatible, as long as the SDK version matches. 
However, developers are encouraged to optimize their units for each target platform, and thus specialized builds should be preferred if available.

drumlogue units are not binary compatible with other platforms.

#### Overall Structure:
* [platform/prologue/](platform/prologue/) : prologue specific files, templates and demo projects.
* [platform/minilogue-xd/](platform/minilogue-xd/) : minilogue xd specific files, templates and demo projects.
* [platform/nutekt-digital/](platform/nutekt-digital/) : Nu:Tekt NTS-1 digital kit specific files, templates and demo projects.
* [platform/drumlogue/](platform/drumlogue/) : drumlogue specific files and templates.
* [platform/ext/](platform/ext/) : External dependencies and submodules.
* [docker/](docker/) : Sources for a docker container that allows building projects for any platform in a more host OS agnostic way.
* [tools/](tools/) : Installation location and documentation for tools required to build projects and manipulate built products. Can be ignored if using the docker container.
* [devboards/](devboards/) : Information and files related to limited edition development boards.

## Sharing your Oscillators/Effects with us

To show us your work please reach out to *logue-sdk@korg.co.jp*.

## Support

The SDK is provided as-is, no technical support will be provided by KORG.

<!-- ## Troubleshooting -->





