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

# Security

In order for ze-monitor to read the performance metric units (PMU) in the 
Linux kernel, it needs elevated permissions. The easiest way is to just
run under sudo (eg., `sudo ze-monitor ...`.)

However, I'm not a fan of running random utilities under sudo.

From [Perf Security](https://www.kernel.org/doc/html/v5.1/admin-guide/perf-security.html) and [man capabilities](https://man7.org/linux/man-pages/man7/capabilities.7.html), the specific capabilities required are:

| Capability          | Reason                                               |
|:--------------------|:-----------------------------------------------------|
| CAP_DAC_READ_SEARCH | Bypass all filesystem read access checks             |
| CAP_PERFMON         | Access to perf_events (vs. overloaded CAP_SYS_ADMIN) |
| CAP_SYS_PTRACE      | PTRACE_MODE_READ_REALCREDS ptrace access mode check  |

To configure ze-monitor to run with those privileges, you can use `setcap` to set the correct capabilities on ze-monitor. You can further create a user group
specifically for running the utility and restrict running of that command to 
users in that group.

## Anyone can run ze-monitor

```bash
sudo setcap "cap_perfmon,cap_dac_read_search,cap_sys_ptrace=ep" ze-monitor
getcap ze-monitor
```

## Only users in group `ze-monitor` can run ze-monitor

```bash
# Create the ze-monitor group,
sudo groupadd ze-monitor
# Set the ze-monitor group to own the utility
# With the following chmod, this will restrict write access to only root
sudo chown root:ze-monitor ze-monitor
# Set permissions on the utility to only be runnable by
# the owner and the group
sudo chmod u+rwx,g+rx-w,o-rwx ze-monitor
# After changing ownership of the ze-utility, set
# the capabilities
sudo setcap "cap_perfmon,cap_dac_read_search,cap_sys_ptrace=ep" ze-monitor
# Add the current user to the group
sudo usermod -aG ze-monitor $(whoami)
# You either need to logout/login, or activate the
# group on the current user
newgrp ze-monitor
```

# Build outside container

## Prerequisites

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
## Building

```
cd build
cmake ..
make
```

## Running

```
build/ze-monitor
```

## Build and install .deb

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
docker compose run --rm ze-monitor build.sh
build/ze-monitor
```

The build.sh script will build the binary in /opt/ze-monitor/build,
which is volume mounted to the host's build directory.

NOTE: See [Security](#security) for information on running ze-monitor
with required kernel access capabilities.

# Running

NOTE: See [Security](#security) for information on running ze-monitor
with required kernel access capabilities.

If running within a docker container, the container environment does not
have access to the host's `/proc/fd`, which is necessary to obtain
information about the processes outside the current container which are
using the GPU. As such, only processes running within that container running
ze-monitor will be listed as using the GPU.

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

NOTE: See [Security](#security) for information on running ze-monitor
with required kernel access capabilities.

## Monitor a given device

```
sudo ze-monitor --device ( PCIID | # | BDF | UUID | /dev/dri/render* ) \
  --interval ms
```

NOTE: See [Security](#security) for information on running ze-monitor
with required kernel access capabilities.

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