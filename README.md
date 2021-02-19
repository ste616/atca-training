# ATCA Training Tools
Tools and documents related to observer and duty astronomer training for the ATCA.

This package has a number of applications that can be used to learn, at your
own pace, about how the Australia Telescope Compact Array (ATCA) works, and how
to interrogate and intepret data coming from the telescope.

## Installation
This software uses the cmake compilation manager; please ensure you have it
installed before attempting to compile this software.

Please also ensure that you install:
* gfortran
* PGPLOT
* [RPFITS library](https://www.atnf.csiro.au/computing/software/rpfits.html)

To checkout and compile this software, you may follow the instructions below.
```bash
git clone https://github.com/ste616/atca-training.git
cd atca-training
mkdir build && cd build
cmake ..
make
```

## Basic usage
In a nutshell, you will want to load up one or more RPFITS files created during
ATCA observations with the `rpfitsfile_server` application, and then look at
the data in that file using `nvis` and `nspd`. With these applications you can
learn how modifying certain parameters can affect the data, and by following
the tutorials and then playing around yourself you can become a more confident
ATCA observer or duty astronomer.

### Usage guides for the tools

[`nvis`](src/apps/nvis/)

Soon, we will put links here to the usage guides for `rpfitsfile_server`, `nvis` and `nspd`.

Soon, we will put links here to individual tutorials.

## About this package
This software was written and tested by Jamie Stevens (ste616@gmail.com, or Jamie.Stevens@csiro.au) in 2020/2021. It is licenced under the GPLv3 open source licence.
