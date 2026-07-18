FROM mcr.microsoft.com/devcontainers/python:1-3.11-bookworm

# Tools needed to build and package a Flipper Zero external app (.fap) with ufbt.
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    curl \
    wget \
    protobuf-compiler \
    && rm -rf /var/lib/apt/lists/*

# ufbt (micro Flipper Build Tool) downloads and manages the ARM toolchain/SDK itself.
RUN pip install --no-cache-dir ufbt
