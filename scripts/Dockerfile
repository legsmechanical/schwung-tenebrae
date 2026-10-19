# Tenebrae build environment: aarch64 cross toolchain for the Move
FROM debian:bookworm
RUN apt-get update && apt-get install -y gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu make file \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /build
ENV CROSS_PREFIX=aarch64-linux-gnu-
