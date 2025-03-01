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
```

## Show details for a given device

```
sudo ze-monitor --info --device ( PCIID | # | BDF | UUID )
```

Example output:

```bash
```

## Monitor a given device

```
sudo ze-monitor --device ( PCIID | # | BDF | UUID ) \
  --interval ms --output ( txt | json | ncurses)
```

If `--one-shot` is listed, --interval will be used as a one-shot
to gather statistics.

Example output: `ze-monitor --device 1 --interval 1000 --output txt`

```bash
```


