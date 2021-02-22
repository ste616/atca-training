# rpfitsfile_server
Server application to provide data from RPFITS files to `nvis` and `nspd`.

## Introduction

Whereas `vis` and `spd` get their data from shared memory on the observing
computers, `nvis` and `nspd` communicate over network sockets. This application
makes it possible to use an existing RPFITS file as the data source for `nvis`
and `nspd`, and to allow the data to be manipulated in certain ways.

## Usage

This is a command-line tool.

### Command-line options

```
 $ ./rpfitsfile_server --help
Usage: rpfitsfile_server [OPTION...] [options] RPFITS_FILES...
RPFITS file reader for network tasks

  -n, --networked            Switch to operate as a network data server 
  -p, --port=PORTNUM         The port number to listen on
  -t, --testing=TESTFILE     Operate as a testing server with instructions
                             given in this file (multiple accepted)
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to <Jamie.Stevens@csiro.au>.
```

In normal operation, `rpfitsfile_server` is run as a network data server,
with the default port number of 8880. In this case, simply start the
application as below:

```bash
rpfitsfile_server -n 2020-02-02_0202.C2020 ...
```

One or more RPFITS files can be supplied as command line arguments, and
`rpfitsfile_server` will allow access to all of them.

### Startup

On startup, you will see a summary of all the scans in each file
supplied, for example:

```
 $ rpfitsfile_server -n 2021-01-16_2246.C999 
RPFITS FILE: 2021-01-16_2246.C999 (1 scans):
  scan 1 (1934-638       , ) MJD range 59230.949363 -> 59230.961747 (108 c)
```

A random cycle from a random scan is then selected and read in as the
data that will be presented to any new `nspd` clients. The entire set of
files is also read and averaged using default options; this data is
presented to any new `nvis` clients.

When these initial reads are finished, the server will begin listening for
connections.