FROM ubuntu:22.04 as builder

ENV DEBIAN_FRONTEND noninteractive

ENV FORCE_UNSAFE_CONFIGURE 1

ENV PATH="/spack/bin:${PATH}"

ENV MPICH_VERSION=3.4.3

ENV CMAKE_VERSION=3.30.3

RUN apt-get -y update

RUN apt-get install -y apt-utils

# install basic tools
RUN apt-get install -y --no-install-recommends gcc g++ gfortran git make unzip file \
  vim wget pkg-config python3-pip python3-dev cython3 python3-pythran curl tcl m4 cpio automake meson \
  xz-utils patch patchelf apt-transport-https ca-certificates gnupg software-properties-common perl tar bzip2 \
  liblzma-dev libbz2-dev

# install CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-aarch64.tar.gz -O cmake.tar.gz && \
    tar zxvf cmake.tar.gz --strip-components=1 -C /usr

# get latest version of spack
RUN git clone -b v0.23.0 https://github.com/spack/spack.git

COPY spack /costa-spack-repo

RUN spack repo add /costa-spack-repo

# set the location of packages built by spack
RUN spack config add config:install_tree:root:/opt/local

# find all external packages
RUN spack external find --all

# find compilers
RUN spack compiler find

# install yq (utility to manipulate the yaml files)
RUN wget -qO /usr/local/bin/yq https://github.com/mikefarah/yq/releases/latest/download/yq_linux_arm64 && chmod a+x /usr/local/bin/yq

# install MPICH
RUN spack install mpich@${MPICH_VERSION} %gcc

# for the MPI hook
RUN echo $(spack find --format='{prefix.lib}' mpich) > /etc/ld.so.conf.d/mpich.conf
RUN ldconfig

# create environments for several configurations and install dependencies
RUN spack env create -d /COSTA-env && \
    spack -e /COSTA-env add "costa@master %gcc build_type=RelWithDebInfo +scalapack +tests +shared ^openblas threads=openmp" && \
    spack -e /COSTA-env develop -p /src costa@master && \
    spack -e /COSTA-env install --only=dependencies --fail-fast
