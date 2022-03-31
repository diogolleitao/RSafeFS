# Get the ubuntu image from Docker Hub
FROM ubuntu:20.04

# Install system dependencies 
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y \
    wget \
    ninja-build \
    meson \
    build-essential \
    autoconf \
    libtool \
    pkg-config \
    cmake \
    udev \
    git \
    libfuse-dev 
