# nvis
A "clone" of ATCA's vis, with extra features.

![nvis example display](nvis_example.png)

## Introduction

The `nvis` application, like the `vis` application it clones and is used
online at the ATCA, allows you to quickly ascertain the quality and properties
of the data. It does this by displaying averaged quantities for different
antennas and baselines, and how they vary over time.

## Usage

This is a command-line tool with a PGPLOT display.

### Command-line options

```
 $ ./nvis --help
Usage: nvis [OPTION...] [options]
new/network VIS

  -d, --device=PGPLOT_DEVICE The PGPLOT device to use
  -f, --file=FILE            Use an output file as the input
  -p, --port=PORTNUM         The port number on the server to connect to
  -s, --server=SERVER        The server name or address to connect to
  -u, --username=USERNAME    The username to communicate to the server
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to <Jamie.Stevens@csiro.au>.
```

In normal operation, `nvis` is connected to an `rpfitsfile_server` which is
providing the data from an RPFITS file. If you're running the server on your
own machine, `nvis` is usually started in the following way.

```bash
./nvis -d /xs -s localhost
```

If the server is not running on your local machine, you can specify the
server address and TCP port number with the `-s` and `-p` options respectively.

If you're trying to use both `nvis` and `nspd` at the same time and have changes
in one be reflected in the other, you can also specify a username with the
`-u` option; any clients with that same username will update automatically
whenever changes are made in any other client. The username can be anything,
but only the first 20 characters are used.

Upon start, you will see something like the following appear in your terminal
(assuming you've connected to an `rpfitsfile_server`):

```
 $ ./nvis -d /xs -s localhost -u abc123
Client ID = sY+x)7CravyYglNFO)e
Connected to SIMULATOR server.
NVIS> 
```

The `Client ID` is a unique 20-character-long string identifying this client,
and is randomly generated at each execution; this is useful primarily for
working out which clients have requested what in the logs from
`rpfitsfile_server`.

The `NVIS>` prompt will appear whenever `nvis` is ready to accept commands.


