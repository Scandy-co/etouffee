# Etouffee

Étouffée (pronounced: `āto͞oˈfā`) is an example app built on [Roux](https://www.scandy.co/apps/roux) for Unix (macOS and Linux) using [Qt](https://www.qt.io/).

NOTE: Roux was formerly called ScandyCore so there is a bit both here as we transition completely to the new name.

- [Etouffee](#etouffee)
  - [Setup](#setup)
  - [Build](#build)
    - [configure](#configure)
    - [cmake](#cmake)
    - [build](#build-1)
  - [Run](#run)
    - [configure](#configure-1)
    - [run](#run-1)

## Setup

macOS:

```bash
brew install qt
```

linux:

```bash
sudo apt install qt5
```

## Build

### configure

macOS

```bash
JOBS=$(sysctl -a | egrep -i "hw.ncpu" | awk -F ':' '{print $2}')
```

linux

```bash
JOBS=$(nproc)
```

### cmake

```bash
export Roux_DIR=$PWD/dependencies/roux
mkdir -p build/install
cd build
cmake \
  -D CMAKE_INSTALL_PREFIX=$PWD/install \
  -D CMAKE_BUILD_TYPE=Release \
  ..
```

### build

```bash
cmake --build . -j $JOBS
```

## Run

### configure

macOS

```bash
export DYLD_LIBRARY_PATH=$PWD:$PWD/lib:$Roux_DIR/lib
```

linux

```bash
export LD_LIBRARY_PATH=$PWD:$PWD/lib:$Roux_DIR/lib
```

### run

```bash
./Etouffee
```
