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

### Data manipulation

While reading the data from the RPFITS files, a set of default options
is created for each different configuration found, with each configuration
being a different set of continuum central frequencies.

Each set of options consists of the following parameters:
* the units of the phase: default is to make phase in units of degrees,
  but can be set to radians
* whether to include flagged data: the default is not to include any
  flagged data in the computations
* how to correct the data for system temperature: by default, the
  data is left as it was written, which normally means that the correlator
  has made some online corrections, but it can also be uncorrected, or
  it can be corrected by the system temperatures that are computed by
  `rpfitsfile_server`
* the tvchannel range: the range of channels to use when averaging the
  data before presenting it to an `nvis` client; this range can be set
  independently for each frequency window, and the default is dependent
  on which frequency band it is
* the delay averaging: the number of channels within the tvchannel range
  to average together before computing the phase which will be used to
  calculate the delay error; this parameter can be set independently for
  each frequency window, and the default is 1
* the averaging method: how to average data across the tvchannel range;
  this parameter can be set indepedently for each frequency window, and by
  default it is set to scalar mean averaging

The clients can send their desired options sets to this server, and the
data will be re-read from the files and new computations performed before
sending the data back. The server also keeps a cache so that if a request
is made with options it has seen before, it can immediately return the
pre-computed data rather than having to re-read it from file.

A cache is also kept for each user, so that if the same user connects
with multiple clients, each of the clients is kept synchronised with
changes to the options made by any one of the clients. For example, if
the tvchannel range is changed by a user via their `nvis` client, their
`nspd` client will also be given new data reflecting the change.

### Shutdown

To stop the server, simply press Ctrl-c in the controlling terminal.
The program will catch the SIGINT and shutdown gracefully.

