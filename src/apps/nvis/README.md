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

### Display

`nvis` shares many of the same display qualities as `vis`, so as to ensure
that if you're familiar with examining `nvis` you'll be at ease
with `vis` when you're actually in control of the telescope.

Some things are different however, in order to make it clearer what is
being displayed. For example, instead of having the source name and central
frequencies of the two "calibration" IFs being jammed up at the top-right
of the display like `vis`, `nvis` shows the IFs being displayed, and their
frequencies underneath that. You may also display more than three panels if
you'd like, which is something that `vis` cannot do. At the moment, `nvis`
does not however have the symbol legend that `vis` does at the top of the display,
but this may come in future updates.

You cannot currently use any x-axis other than time with `nvis` either, but
this will also be rectified as development continues; it is intended that
`nvis` will be used in the BIGCAT era.

To change which panels `nvis` displays, you can give a panel specifier in much
the same way as `vis`. For example, the following two commands are identical,
and will cause both `vis` and `nvis` to show the "Amplitude", "Phase" and
"Delay" plots (in that order, from top to bottom), with "UT" along the x-axis.

```
apd-t
```

```
tapd
```

In the first example, the x-axis is given as the last character, after the
`-`, while in the first example the x-axis is given as the first character.
Each other character instructs `nvis` about which panels to display, in order
from top to bottom; in this case "a" (Amplitude), "p" (Phase) and "d" (Delay).

The complete list of all panels available to display in `nvis` is given in the
table below, along with a brief description if necessary. This list is given in
alphabetical order, and each character specification is case-sensitive.

Character | Panel Displayed
--------- | ---------------
a         | Amplitude: the average amplitude on each baseline per cycle
C         | Computed System Temperature: the system temperature of each antenna, as computed using the options currently supplied by this client
d         | Delay: the delay error as determined by examining the phases as a function of frequency
D         | Wind Direction: the direction the wind is coming from
G         | GTP: the gated total power measured online
H         | Humidity: as reported by the site weather station
n         | The amplitude of the noise diode in Jy: this is measured per antenna
N         | SDO: the synchronously-demodulated output measured online
p         | Phase: the average phase on each baseline per cycle
P         | Pressure: as reported by the site weather station
R         | Rain Gauge: as reported by the site weather station
S         | System Temperature: the system temperature of each antenna, as computed online by the correlator using the options that were set during the observation
t         | Time: the UTC, can be used as an x-axis
T         | Temperature: as reported by the site weather station
V         | Wind Speed: as reported by the site weather station
X         | Seeing Monitor Phase: the phase measured on the seeing monitor interferometer
Y         | Seeing Monitor RMS Phase: the phase RMS noise measured on the seeing monitor interferometer

For each panel, a label indicating what it is can be found on the left of the
display, and the units being plotted on the right. The axis scaling is usually
alternated between the left and right to ensure that the numbers don't collide
between adjacent panels.

At the bottom of the display is the colour key between baselines, polarisations
and colours. Up to 16 different lines can be plotted, and for all panels which
display baselines (even auto-correlated baselines), the colour key will apply.

For measurements that do not depend on baselines (like the weather station
parameters), only a single line will be shown, and it will usually be white-ish.

For parameters that depend on antenna only (like system temperatures), the line
colours are specified by the antenna colours shown next to the "Ants:" label at
the top of the display. In addition, as shown in the image below, a key between
line style and polarisation will be displayed to the right of the antenna colour
key.

![nvis display with antenna-based parameters](nvis_antenna_display.png)

### Interactive commands

Like `vis`, what `nvis` displays is controlled by commands given at the
`NVIS>` prompt. This section describes the commands that `nvis` understands.

Each command can be given with minimum match; the minimum string is given
for each command below in **bold**. For example, you may give the **sel**ect command
as `sel` or `select`, or even `sele`, but not `se`.

#### select

Format: **sel**ect [*products*]

