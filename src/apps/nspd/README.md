# NSPD
A "clone" of ATCA's SPD, with extra features.

![nspd example display](nspd_example.png)

## Introduction

The `nspd` application, like the `spd` application it clones and is used
online at the ATCA, allows you to view the raw data coming from the
telescope. It does this by displaying spectra for different antennas and
baselines for each integration.

## Usage

This is a command-line tool with a PGPLOT display.

### Command-line options

```
 $ ./nspd --help
Usage: nspd [OPTION...] [options]
new/network SPD

  -d, --device=PGPLOT_DEVICE The PGPLOT device to use
  -D, --default-dump=DUMP_TYPE   The plot type to use as default for output
                             files (default: PNG)
  -f, --file=FILE            Use an output file as the input
  -p, --port=PORTNUM         The port number on the server to connect to
  -s, --server=SERVER        The server name or address to connect to
  -u, --username=USERNAME    The username to communicate to the server
  -v, --verbose              Output debugging information
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to <Jamie.Stevens@csiro.au>.
```

In normal operation, `nspd` is connected to an `rpfitsfile_server` which is
providing the data from an RPFITS file. If you're running the server on your
own machine, `nspd` is usually started in the following way.

```bash
./nspd -d /xs -s localhost
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
 $ ./nspd -d /xs -s localhost -u abc123
client ID = ,.^SdV4SIaOh-_'40wE
Connected to SIMULATOR server.
NSPD> 
```

The `Client ID` is a unique 20-character-long string identifying this client,
as is randomly generated at each execution; this is useful primarily for
working out which clients have requested what in the logs from
`rpfitsfile_server`.

The `NSPD>` prompt will appear whenever `nspd` is ready to accept commands.

### Display

`nspd` shares many of the display qualities as `spd`, so as to ensure that
if you're familiar with examining `nspd` you'll be at ease with `spd`
when you're actually in control of the telescope.

There are some differences however, in order to make it clearer what is
being displayed. For example, there is an area at the top of the display
which shows the time label for the data being shown, some environmental
data, and the system temperatures.

### Interactive commands

Like `spd`, what `nspd` displays is controlled by commands given at the
`NSPD>` prompt. This section describes the commands that `nspd` understands.

Each command can be given with minimum match; the minimum string is given
for each command below in **bold**. For example, you may give the **sel**ect
command as `sel` or `select`, or even `sele`, but not `se`.

#### amplitude

Format: **a**mplitude [*min amplitude* *max amplitude*]

This command makes `nspd` display amplitude as the y-axis of each of the
panels. If you would like to limit the range of each panel, you can specify
both the *min amplitude* and *max amplitude* arguments. This will limit all
the panels of all the IFs in the same way.

If you want to reset the amplitude scaling to the default (which will be to
contain all the data on each panel, which likely means that each panel has
its own range), give this command without arguments.

The range should be given on the linear axis values, even if plotting the amplitude
on a logarithmic scale.

#### array

Format: **arr**ay *antennas*

This command selects which antennas are available to display. You can
specify which antennas to include either as a list or a string.
For example: `arr 1 2 3 4 5` would make `nspd` show any antenna product
or baseline which only included antennas 1 through 5 inclusive. This is
equivalent to the more traditional `spd` format of `array 12345`.

#### backward

Format: **back**ward

This command instructs the server to supply the data from the cycle
immediately prior to the currently displayed data.

While the server retrieves the data from file, `nspd` will continue to
show the current data.

#### channel

Format: **ch**annel [[*IF name*] *min channel* *max channel*]

Change the channel range shown on each panel. If no arguments are given
along with this command, then all the channels for all IFs are shown.

If given with two arguments, then all panels for all IFs are shown with
the same range, which is specified by the *minimum* and *maximum* channel
arguments. Both these arguments are inclusive, such that if the command
`chan 1 100` is given, all channels from 1 to 100 (and including channels
1 and 100) are shown on the panels.

If given with three arguments, the first argument specifies the IF to
restrict. The IF can be specified in several ways:
- f*n*: where f1 is the first continuum band, and f3 is usually the
  first zoom band, and *n* can continue up until however many zooms
  are in the data + 2
- z*n*: where z1 is the first zoom band, and *n* can continue up until
  however many zooms are in the data
- z*m*-*n*: where z1-2 is the second zoom band associated with the first
  continuum band, and z2-5 is the fifth zoom band associated with the second
  continuum band; here *m* can be either 1 or 2, and *n* is between 1 and 16
  inclusive, depending on how many zooms are configured

Note that you cannot unrestrict a single IF with this command, ie.
`chan f1` will not do anything. If you need to reset the channel range
to default for one IF, you must do it for all IFs.

#### delavg

Format: **delav**g [*band*] *number of channels*

This command instructs the server to recompute data while performing
averaging over phase while calculating the delay errors.

If only one argument is supplied, the *number of channels* specified
will be used as the averaging level for all the IFs available in the
data. If two arguments are supplied, the first argument specifies
the IF to change the averaging level for. The IF can be specified as for
the **chan**nel command.

While the server recomputes the data, `nspd` will continue to show the
current data.

Note that the phase, amplitude, real and imaginary spectra shown by
`nspd` will not be affected by changing this setting. The only thing
that will differ will be the averaged phase quantity available through
the **sho**w **av**eraged command. Changing this parameter will affect
any `nvis` clients connected with the same username however.

#### dump

Format: **dump** [*filename*]

Output the current plot to a file. If no argument is specified, then the
filename is generated automatically to be
`nspd_plot_yyyy-mm-dd_HHMMSS`, where `yyyy` is the year (like 2021),
`mm` is the month (like 06), `dd` is the date (like 02), `HH` is the
hour (like 23), `MM` is the minute and `SS` is the second. The time is
in the local timezone according to the machine that `nspd` is running on.

`nspd` can output two types of files: PNG and PS. If the filename is
generated automatically, the type of file created will depend on the
value of the argument supplied to the `-D` option when `nspd` was started.
If no `-D` option was specified, the default PNG will be output. To output
PS without specifying an argument, run `nspd -D ps`.

If a *filename* argument is supplied to the **dump** command, the type of
output will be determined by the extension of that *filename*. If no extension
is supplied (or an extension which isn't `.png` or `.ps`) then the default
file type will be used, exactly as above. If an extension is supplied, the
appropriate file type will be used.

When this command is executed, some output will be given in the terminal
telling you what file is created. For example:

```
NSPD> dump newtest
 NSPD output to file newtest.png
 ```

#### exit

Format: **exit**

Exit `nspd`.

#### forward

Format: **forw**ard

This command instructs the server to supply the data from the cycle
immediately subsequent to the currently displayed data.

While the server retrieves the data from file, `nspd` will continue to
show the current data.

#### get

Format: **get** *quantity* *arguments*

Get some *quantity* from the server. The options for *quantity* are:

##### time

Format: **get** **tim**e [*yyyy-mm-dd*] *HH:MM*[*:SS*]

Get a new cycle of data from the server, specified by a time and optional
date. The date will need to be specified only if the data coming from the
server spans more than one day.

While specifying the time, only the hours and minutes are necessary; if the
seconds are omitted, "00" seconds is assumed.

The server will return data for the cycle whose mid-cycle time is nearest
to the specified time. While the server retrieves the data, `nspd` will
continue to show the current data.

#### hide

Format: **hid**e _**tvch**annels/**av**eraged_

#### imaginary

Format: **i**maginary [*min imaginary* *max imaginary*]

#### list

Format: **lis**t

#### nxy

Format: **nxy** *nx* *ny*

#### off

Format: **off** *product* [*product* ...]

#### on

Format: **on** *product* [*product* ...]

#### phase

Format: **p**hase [*min phase* *max phase*]

#### quit

Format: **quit**

Exit `nspd`.

#### real

Format: **r**eal [*min real* *max real*]

#### scale

Format: **sca**le _**log**arithmic/**lin**ear_

#### select

Format: **sel**ect *product* [*product* ...]

This command selects which products to show in each panel. You may specify
any number of products in a single command. These products are the polarisations
codes **aa**, **bb**, **ab**, **ba**, or one of the band names, like **f1**,
**f2**, **z1** etc.

Only products that have been enabled with the **on** command will be displayed.
For example, normally **ab** and **ba** are not enabled for cross-correlations
and thus will not be displayed even if selected with this command.

#### show

Format: **sho**w _**tvch**annels/**av**eraged_

#### x

Format: **x**

