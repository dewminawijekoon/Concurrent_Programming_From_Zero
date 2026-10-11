#!/usr/bin/env bash
# install.sh - install everything this repo needs on Ubuntu / Debian / WSL2.
# Run:  bash setup/install.sh      (asks for your sudo password)
# macOS / Windows without WSL: use the dev container instead (see README).
set -e
if ! command -v apt-get >/dev/null; then
    echo "This script supports apt-based systems (Ubuntu, Debian, WSL2)."
    echo "On other systems, use the dev container described in README.md."
    exit 1
fi
echo "==> Installing compilers, debuggers, OpenMP and MPI"
sudo apt-get update
sudo apt-get install -y build-essential gdb valgrind make python3 \
    libomp-dev openmpi-bin libopenmpi-dev

echo "==> Checking for an NVIDIA GPU (needed only for module 12, CUDA)"
if command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi >/dev/null 2>&1; then
    if command -v nvcc >/dev/null 2>&1; then
        echo "    CUDA toolkit already installed: $(nvcc --version | tail -1)"
    else
        echo "    NVIDIA GPU found. Installing the CUDA toolkit from Ubuntu's repositories"
        echo "    (for the newest version, follow https://developer.nvidia.com/cuda-downloads)"
        sudo apt-get install -y nvidia-cuda-toolkit
    fi
else
    echo "    No NVIDIA GPU detected: skipping CUDA."
    echo "    Module 12 explains how to run the CUDA exercises free on Google Colab."
fi
echo
bash "$(dirname "$0")/check.sh"
