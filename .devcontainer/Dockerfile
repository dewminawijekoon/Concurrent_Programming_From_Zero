# Everything the course needs except CUDA (use Colab for module 12).
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential gdb valgrind make python3 git ca-certificates \
        libomp-dev openmpi-bin libopenmpi-dev \
    && rm -rf /var/lib/apt/lists/*
# OpenMPI refuses to start more processes than cores unless told otherwise.
ENV OMPI_MCA_rmaps_base_oversubscribe=1
RUN useradd -m -s /bin/bash student
USER student
WORKDIR /workspaces
