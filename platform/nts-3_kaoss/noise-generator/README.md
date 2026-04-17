## Noise Generator Build Guide

This folder contains a white noise generator module for the KORG NTS-3 kaoss pad kit.

### Overview

The noise generator generates Gaussian white noise with a controllable output level. It is designed to be built using the logue SDK Docker-based environment.

### Prerequisites

Before building this module, ensure you have the logue SDK environment set up. Refer to the [main Docker README](../../../docker/README.md) for detailed setup instructions.

1. [Install Docker](https://docs.docker.com/get-docker/)
2. Clone the [logue-sdk](https://github.com/korginc/logue-sdk) repository.
3. Build or pull the Docker container image as described in the main documentation.

### Building the Module

You can build this module using the convenience scripts provided in the SDK's `docker` directory.

#### Using `run_cmd.sh`

From the root of the `logue-sdk` repository, execute:

```bash
$ ./docker/run_cmd.sh build nts-3_kaoss/noise-generator
```

This will:
1. Initialize the NTS-3 kaoss development environment inside the container.
2. Compile the source files.
3. Link and strip the final unit file.
4. Output the built files to the `build/` directory within this folder.

#### Using Interactive Shell

If you are performing multiple builds, the interactive shell is more efficient:

```bash
$ ./docker/run_interactive.sh
user@logue-sdk $ env nts-3_kaoss
[nts-3_kaoss] ~ $ cd platform/nts-3_kaoss/noise-generator
[nts-3_kaoss] ~/platform/nts-3_kaoss/noise-generator $ make
```

### Output Files

After a successful build, the following important files will be generated in the `build/` directory:

- `noise_generator.nts3unit`: The final unit file to be loaded onto the NTS-3.
- `noise_generator.hex` / `noise_generator.bin`: Raw firmware files.

### Installation

To install the built unit onto your NTS-3:

1. Connect your NTS-3 to your computer via USB.
2. Use the [logue-cli](https://github.com/korginc/logue-cli) or the KORG Kontrol Editor (if supported) to load the `.nts3unit` file.

Alternatively, if you are using the `build` command, you can specify an installation directory:

```bash
$ ./docker/run_cmd.sh build --install-dir=/path/to/your/units nts-3_kaoss/noise-generator
```

### Cleaning the Build

To remove build artifacts:

```bash
$ make clean
```
or via Docker:
```bash
$ ./docker/run_cmd.sh build -c nts-3_kaoss/noise-generator
```
