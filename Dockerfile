FROM ubuntu:oracular

SHELL [ "/bin/bash", "-c" ]

# Instructions Dockerfied from:
#
# https://github.com/pytorch/pytorch
#
# and
#
# https://pytorch.org/docs/stable/notes/get_start_xpu.html
# https://www.intel.com/content/www/us/en/developer/articles/tool/pytorch-prerequisites-for-intel-gpu/2-6.html
# 
#
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    debhelper \
    devscripts \
    gdb \
    git \
    gpg \
    inotify-tools \
    less \
    libfmt-dev \
    libncurses-dev \
    rpm \
    rpm2cpio \
    wget \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log}

# Install dev requirements for ze-monitor
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    software-properties-common \
    && add-apt-repository -y ppa:kobuk-team/intel-graphics \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libze-intel-gpu1 \
    libze1 \
    libze-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log}

RUN echo -e '#!/bin/sh\n$@' > /usr/local/bin/sudo && \
    chmod +x /usr/local/bin/sudo

COPY /src/* /opt/ze-monitor/
WORKDIR /opt/ze-monitor
