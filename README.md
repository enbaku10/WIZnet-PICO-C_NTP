# Getting Started with NTP Server Examples

These sections will guide you through a series of steps from configuring development environment to running ethernet examples using the **WIZnet's ethernet products** and **Waveshare's GNSS products**.

- [Getting Started with NTP Server Examples](#getting-started-with-ntp-server-examples)
  - [Development environment configuration](#development-environment-configuration)
  - [Hardware requirements](#hardware-requirements)
  - [NTP Server example structure](#ntp-server-example-structure)
  - [NTP Server example testing](#ntp-server-example-testing)
  - [How to use port directory](#how-to-use-port-directory)



<a name="development_environment_configuration"></a>
## Development environment configuration

To test the NTP server examples, the development environment must be configured to use W6300-EVB-Pico2 and Pico-GPS-L76K.

These examples were tested after configuring the development environment on **Windows**. Please refer to '**Chapter 3: Installing the Raspberry Pi Pico VS Code Extension**' in the document below and configure accordingly.

- [**Getting started with Raspberry Pi Pico**][link-getting_started_with_raspberry_pi_pico]

**Visual Studio Code** was used during development and testing of ethernet examples, the guide document in each directory was prepared also base on development with Visual Studio Code. Please refer to corresponding document.


<a name="WIZnet Raspberry Pi Pico Board List"></a>
## Hardware requirements

The NTP Server examples are compatible with the following Raspberry Pi-compatible WIZnet Ethernet I/O module and Raspberry Pi Pico GNSS (Global Navigation Satellite System) expansion board module.

Raspberry Pi-compatible WIZnet Ethernet I/O module integrates [**WIZnet Ethernet chips**][link-wiznet_ethernet_chips] **W6300** with [**RP2350**][link-rp2350] microcontroller.

| Board/Module Name              | MCU      | Ethernet Chip  | Interface     | Socket # | TX/RX Buffer  | Notes                                  |
|--------------------------------|----------|----------------|---------------|----------|---------------|----------------------------------------|
| **[W6300-EVB-Pico2][link-w6300-evb-pico2]** | RP2350 | W6300 | QSPI (PIO) | 8 | 64KB | Supports IPv4/IPv6 |

Raspberry Pi Pico GNSS expansion board module integrates **L76K**.

| Board/Module Name              | Chip  | Interface     | Communication protocol  | Notes                                  |
|--------------------------------|----------------|---------------|---------------|----------------------------------------|
| **[Pico-GPS-L76K][link-pico-gps-l76k]** | L76K | UART | NMEA 0183, PMTK | Supports Multi-GNSS systems: GPS, BeiDou (BDS), GLONASS and QZSS |


<a name="ntp_server_example_structure"></a>
## NTP Server example structure

Examples are available at '**WIZnet-PICO-C/examples/ntp**' directory. 

Note that **ioLibrary_Driver**, **mbedtls**, **pico-sdk** are needed to run ethernet examples.

- **ioLibrary_Driver** library is applicable to WIZnet's WIZchip ethernet chip.
- **mbedtls** library supports additional algorithms and support related to SSL and TLS connections.
- **pico-sdk** is made available by Pico to enable developers to build software applications for the Pico platform.

Libraries are located in the '**WIZnet-PICO-C/libraries/**' directory.

- [**ioLibrary_Driver**][link-iolibrary_driver]
- [**mbedtls**][link-mbedtls]
- [**pico-sdk**][link-pico_sdk]

If you want to modify the code that MCU-dependent and use a MCU other than **RP2040**, you can modify it in the '**WIZnet-PICO-C/port/**' directory.

port is located in the '**WIZnet-PICO-C/port/**' directory.

- [**ioLibrary_Driver**][link-port_iolibrary_driver]
- [**mbedtls**][link-port_mbedtls]
- [**timer**][link-port_timer]

The structure of this WIZnet-PICO-C 2.0.0 version or higher has changed a lot compared to the previous version. If you want to refer to the previous version, please refer to the link below.

- [**WIZnet-PICO-C 1.0.0 version**][link-wiznet_pico_c_1_0_0_version]



<a name="ntp_server_example_testing"></a>
## NTP Server example testing

1. Download

If the ethernet examples are cloned, the library set as a submodule is an empty directory. Therefore, if you want to download the library set as a submodule together, clone the ethernet examples with the following Git command.

```cpp
/* Change directory */
// change to the directory to clone
cd [user path]

// e.g.
cd D:/WIZnet-PICO_

/* Clone */
git clone --recurse-submodules https://github.com/enbaku10/WIZnet-PICO-C_NTP.git
```

With Visual Studio Code, the library set as a submodule is automatically downloaded, so it doesn't matter whether the library set as a submodule is an empty directory or not, so refer to it.

[link-getting_started_with_raspberry_pi_pico]: https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf
[link-rp2350]: https://www.raspberrypi.com/products/rp2350/
[link-w6300]: https://docs.wiznet.io/Product/iEthernet/W6300/overview
[link-wiznet_ethernet_chips]: https://docs.wiznet.io/Product/iEthernet#product-family
[link-iolibrary_driver]: https://github.com/Wiznet/ioLibrary_Driver
[link-mbedtls]: https://github.com/ARMmbed/mbedtls
[link-pico_sdk]: https://github.com/raspberrypi/pico-sdk
[link-port_iolibrary_driver]: https://github.com/WIZnet-ioNIC/WIZnet-PICO-C/tree/main/port/ioLibrary_Driver
[link-port_mbedtls]: https://github.com/WIZnet-ioNIC/WIZnet-PICO-C/tree/main/port/mbedtls
[link-port_timer]: https://github.com/WIZnet-ioNIC/WIZnet-PICO-C/tree/main/port/timer
[link-wiznet_pico_c_1_0_0_version]: https://github.com/WIZnet-ioNIC/WIZnet-PICO-C/tree/1.0.0
[link-w6300-evb-pico2]: https://docs.wiznet.io/Product/iEthernet/W6300/w6300-evb-pico2
[link-w6300]: https://docs.wiznet.io/Product/iEthernet/W6300
[link-pico-gps-l76k]: https://www.waveshare.com/wiki/Pico-GPS-L76K


