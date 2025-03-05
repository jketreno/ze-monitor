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
sudo apt install libze1 libncurses6
version=$(cat src/version.txt)
docker compose run --remove-orphans --rm \
  ze-monitor \
  cp /opt/ze-monitor-static/build/ze-monitor-${version}_amd64.deb \
  /opt/ze-monitor/build
sudo dpkg -i build/ze-monitor-${version}_amd64.deb
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
cd build
cmake ..
make
```

### Running

```
build/ze-monitor
```

### Build and install .deb

In order to build the .deb package, you need the following
packages installed:

```bash
sudo apt-get install -y \
    debhelper \
    devscripts \
    rpm \
    rpm2cpio
```

You can then build the .deb:

```bash
if [ -d build ]; then
  cd build
fi
version=$(cat ../src/version.txt)
cpack
sudo dpkg -i build/packages/ze-monitor_${version}_amd64.deb
```

You can then run ze-monitor from your path:

```bash
ze-monitor
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
sudo ze-monitor --info --device ( PCIID | # | BDF | UUID | /dev/dri/render*)
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

```
sudo ze-monitor --device ( PCIID | # | BDF | UUID | /dev/dri/render* ) \
  --interval ms
```

Output:

```bash
$ sudo ze-monitor --device 2 --interval 500
Device: 8086:E20B (Intel(R) Graphics [0xe20b])
Total Memory:  12809404416
Free memory:  [#  55% ############################                              ]
Power usage: 165.0W
------------------------------------------------------------------------------------------
   PID COMMAND-LINE
       USED MEMORY       SHARED MEMORY     ENGINE FLAGS
------------------------------------------------------------------------------------------
     1 /sbin/init splash
       MEM: 106102784    SHR: 100663296    FLAGS: RENDER COMPUTE
  1606 /usr/lib/systemd/systemd-logind
       MEM: 106102784    SHR: 100663296    FLAGS: RENDER COMPUTE
  5164 /usr/bin/gnome-shell
       MEM: 530513920    SHR: 503316480    FLAGS: RENDER COMPUTE
  5237 /usr/bin/Xwayland :1024 -rootless -nores...isplayfd 6 -initfd 7 -byteswappedclients
       MEM: 0            SHR: 0            FLAGS:
 40480 python chat.py
       MEM: 5544226816   SHR: 0            FLAGS: DMA COMPUTE
```

If you pass `--one-shot`, statistics will be gathered, displayed, and
then ze-monitor will exit.