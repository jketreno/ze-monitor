# ze-monitor

A small utility to monitor Level Zero devices via 
[Level Zero Sysman](https://oneapi-src.github.io/level-zero-spec/level-zero/latest/sysman/PROG.html#sysman-programming-guide) 
from the command line, similar to 'top'.

# Installation

This project uses docker containers to build. As this was originally
written to monitor an Intel Arc B580 (Battlemage), it requires a
kernel that supports that hardware, such as the one documented
at [Intel Graphics Preview](https://github.com/canonical/intel-graphics-preview), which runs in Ubuntu Oracular (24.10). It will
monitor any Level Zero device, even those using the i915 driver.

NOTE: You need 'docker compose' installed. See [Install Docker Engine on Ubuntu](https://docs.docker.com/engine/install/ubuntu/)

```
git clone https://github.com/jketreno/ze-monitor.git
cd ze-monitor
docker compose build
docker compose run --rm ze-monitor make deb
sudo apt install libze1 libncurses6
version=$(cat src/version.txt)
sudo dpkg -i src/build/${version}/packages/ze-monitor_${version}_amd64.deb
```

## Build outside container

### Prerequisites

If you would like to build outside of docker, you need the following packages
installed:

```
sudo apt-get install -y \
    build-essential \
    libfmt-dev \
    libncurses-dev
```

In addition, you need the Intel drivers installed, which are available from the
`kobuk-team/intel-graphics` PPA:

```
sudo apt-get install -y \
    software-properties-common \
    && sudo add-apt-repository -y ppa:kobuk-team/intel-graphics \
    && sudo apt-get update \
    && sudo apt-get install -y \
    libze-intel-gpu1 \
    libze1 \
    libze-dev
```
### Building

```
cd src
make
```

### Running

```
version=$(cat version.txt)
build/${version}/ze-monitor
```

# Developing

To run the built binary without building a full .deb package, you can
build and run on the host by compiling in the container:

```
docker compose run --rm ze-monitor make
version=$(cat src/version.txt)
src/build/${version}/ze-monitor
```

NOTE: Due to needing process maps from /proc, ze-monitor can not be run
within a docker container with process monitoring.

# Running

In order to access the GPU driver engines and process maps, ze-monitor needs
to be run as root (via sudo)

## List available devices

```
ze-monitor
```

Example output:

```bash
$ ze-monitor 
Device 1: 8086:E20B (Intel(R) Graphics [0xe20b])
Device 2: 8086:A780 (Intel(R) UHD Graphics 770)
```

## Show details for a given device

```
sudo ze-monitor --info --device ( PCIID | # | BDF | UUID )
```

Example output:

```bash
$ sudo ze-monitor --device 2 --info
Device: 8086:A780 (Intel(R) UHD Graphics 770)
 UUID: 868080A7-0400-0000-0002-000000000000
 BDF: 0000:0000:0002:0000
 PCI ID: 8086:A780
 Subdevices: 0
 Serial Number: unknown
 Board Number: unknown
 Brand Name: unknown
 Model Name: Intel(R) UHD Graphics 770
 Vendor Name: Intel(R) Corporation
 Driver Version: 0CB7EFCAD5695B7EC5C8CE6
 Type: GPU
 Is integrated with host: Yes
 Is a sub-device: No
 Supports error correcting memory: No
 Supports on-demand page-faulting: No
 Engines: 7
  Engine 1: ZES_ENGINE_GROUP_RENDER_SINGLE
  Engine 2: ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE
  Engine 3: ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE
  Engine 4: ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE
  Engine 5: ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE
  Engine 6: ZES_ENGINE_GROUP_COPY_SINGLE
  Engine 7: ZES_ENGINE_GROUP_MEDIA_ENHANCEMENT_SINGLE
 Temperature Sensors: 0
```

NOTE: If you run it without 'sudo', no engines will be reported.

## Monitor a given device

WIP

```
sudo ze-monitor --device ( PCIID | # | BDF | UUID ) \
  --interval ms --output ( txt | json | ncurses)
```

If `--one-shot` is listed, --interval will be used as a one-shot
to gather statistics.

```bash
$ ze-monitor --device 1 --interval 1000 --output txt
```


