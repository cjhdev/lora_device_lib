FROM debian:9

# the latest mbed requirements
ARG mbed_requirements_url=https://raw.githubusercontent.com/ARMmbed/mbed-os/master/requirements.txt

RUN dpkg --add-architecture i386 \
  && DEBIAN_FRONTEND=noninteractive apt-get update -y -q \
  && DEBIAN_FRONTEND=noninteractive apt-get upgrade -y -q \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y -q \
    python python-pip \
    git mercurial \
    wget \
  && pip install mbed-cli \
  && wget -O /tmp/mbed-cli-requirements.txt $mbed_requirements_url \
  && pip install -r /tmp/mbed-cli-requirements.txt

ARG toolchain_name=gcc-arm-none-eabi-9-2020-q2-update
ARG toolchain_url=https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2?revision=05382cca-1721-44e1-ae19-1e7c3dc96118&la=en&hash=D7C9D18FCA2DD9F894FD9F3C3DC9228498FA281A

# get the toolchain
RUN cd /opt && wget $toolchain_url -O - | tar xj

ENV GCC_ARM_PATH="/opt/$toolchain_name/bin"
ENV PATH="/opt/$toolchain_name/bin:${PATH}"

# this doesn't seem to work
#RUN mbed config --global toolchain GCC_ARM

WORKDIR /mbed
VOLUME /mbed

ENTRYPOINT [ "mbed" ]
