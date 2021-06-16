# ATCA Training Tools
Tools and documents related to observer and duty astronomer training for the ATCA.

This package has a number of applications that can be used to learn, at your
own pace, about how the Australia Telescope Compact Array (ATCA) works, and how
to interrogate and intepret data coming from the telescope.

## Installation
This software uses the cmake compilation manager; please ensure you have it
installed before attempting to compile this software.

Please also ensure that you install:
* gcc
* gfortran
* PGPLOT
* libreadline
* libX11
* libpng
* argp (Darwin only)

On Ubuntu, these necessary libraries and tools can be installed with:
```bash
sudo apt install build-essential cmake gfortran pgplot5 libreadline-dev libx11-dev libpng-dev
```

On OSX Darwin, they can be installed using Homebrew with:
```bash
brew install gcc gfortran cmake libreadline libx11 libpng16 argp-standalone
```

Unfortunately, PGPLOT is no longer in the Homebrew cellar. You can get it from
MacPorts though.

The code also relies on the libraries:
* [RPFITS library](https://www.atnf.csiro.au/computing/software/rpfits.html)
* [CMP library](https://github.com/camgunz/cmp)
* [CMP mem access library](https://github.com/Stapelzeiger/cmp_mem_access)

However, these libraries are included and compiled with the code in this package.

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

[`rpfitsfile_server`](src/apps/rpfitsfile_server/)
[`nvis`](src/apps/nvis/)
[`nspd`](src/apps/nspd/)

## Tutorials

The following tutorials are designed to help you understand the data that
the Compact Array Broadband Backend (CABB) creates, and how to assess their
quality. Once you master the use of these tools and understand the data,
an actual ATCA observation should be much easier!

### Tutorial 1

This tutorial will introduce you to how to run the software in this package,
and run through a typical 4cm online calibration routine. It will also introduce
some key concepts about how the correlator works.

[Start tutorial 1](tutorials/1_introduction/)

### Tutorial 2

This tutorial will describe in detail the goals of delay calibration, how the
corrections are computed and applied. You'll also learn how to avoid RFI, and
increase signal-to-noise ratio to successfully calibrate the telescope delays.

[Start tutorial 2](tutorials/2_dcal_and_delavg/)


## About this package
This software was written and tested by Jamie Stevens (ste616@gmail.com, or
Jamie.Stevens@csiro.au) in 2020/2021. It is licenced under the GPLv3 open
source licence.
