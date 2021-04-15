# Tutorial 1
An introduction to the software, and understanding online calibration.

## Goals of the tutorial

In this tutorial, you will learn:

* how to use [`rpfitsfile_server`](../../src/apps/rpfitsfile_server/),
  [`nvis`](../../src/apps/nvis/) and [`nspd`](../../src/apps/nspd),
* how the three applications interact with each other,
* what a typical 4cm CABB online calibration looks like.

After completing this tutorial, you will be able to use the tools to learn more
in the other tutorials.

## Getting the tools

We begin by getting and compiling the software. Instructions for doing this
are on the [front page](../../README.md) of this repository, under the "Installation"
heading. After you have successfully compiled the software, you can continue
this tutorial.

## Starting the tools for this tutorial

Open up three terminals. In one of these terminals, navigate to the path
`atca-training/tutorials/1_introduction` and in the other two go to
`atca-training/build`.

You should download the RPFITS file that we will use for this tutorial
from our website:
[2021-01-16_2220.C999](https://www.narrabri.atnf.csiro.au/people/Jamie.Stevens/atca-training-tutorials/1_introduction/2021-01-16_2220.C999). Put it into the
`atca-training/tutorials/1_introduction` directory.

From that same directory, start the `rpfitsfile_server` with the command:
```bash
../../build/rpfitsfile_server -n 2021-01-16_2220.C999
```

The server will load the data in the file over the next few seconds and output
something like the following to the terminal:
```
 $ ../../build/rpfitsfile_server -n 2021-01-16_2220.C999 
RPFITS FILE: 2021-01-16_2220.C999 (1 scans):
  scan 1 (1934-638       , ) MJD range 59230.931307 -> 59230.936747 (48 c)

Preparing for operation...
 grabbing from random scan 0 from file 0, MJD 59230.931423
[data_reader] checking for cached vis products...
[data_reader] no cache hit
[add_client_spd_data] making new client cache DEFAULT
[add_cache_spd_data] adding SPD cache entry for data at time 2021-01-16  22:21:15 with options:
Options set has 1 elements:
  SET 0:
     PHASE IN DEGREES: YES
     INCLUDE FLAGGED: NO
     TSYS CORRECTION: CORRELATOR
     # WINDOWS: 2
     --WINDOW 1:
        CENTRE FREQ: 5500.0 MHz
        BANDWIDTH: 2048.0 MHz
        # CHANNELS: 2049
        TVCHAN RANGE: 513 - 1537
        DELAY AVERAGING: 1
        AVERAGING METHOD: VECTOR MEAN
     --WINDOW 2:
        CENTRE FREQ: 9000.0 MHz
        BANDWIDTH: 2048.0 MHz
        # CHANNELS: 2049
        TVCHAN RANGE: 513 - 1537
        DELAY AVERAGING: 1
        AVERAGING METHOD: VECTOR MEAN
Configuring network server...
Creating socket...
Binding sockets...
   to (null)
Listening...
Waiting for connections...
```

When you see the `Waiting for connections...` output, you can start using the other
tools.