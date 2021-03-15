# NSPD
A "clone" of ATCA's SPD, with extra features.

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

### Interactive commands

Like `spd`, what `nspd` displays is controlled by commands given at the
`NSPD>` prompt. This section describes the commands that `nspd` understands.

Each command can be given with minimum match; the minimum string is given
for each command below in **bold**. For example, you may give the **sel**ect
command as `sel` or `select`, or even `sele`, but not `se`.

#### amplitude

Format: **a**mplitude [*min amplitude* *max amplitude*]

#### array

Format: **arr**ay *antennas*

This command selects which antennas are available to display. You can
specify which antennas to include either as a list or a string.
For example: `arr 1 2 3 4 5` would make `nspd` show any antenna product
or baseline which only included antennas 1 through 5 inclusive. This is
equivalent to the more traditional `spd` format of `array 12345`.

#### backward

Format: **back**ward

#### channel

Format: **ch**annel [[*band*] *min channel* *max channel*]

#### delavg

Format: **delav**g [*band*] *number of channels*

#### forward

Format: **forw**ard

#### get

Format: **get** _**tim**e_ *arguments*

#### hide

Format: **hid**e _**tvch**annels/**av**eraged**_

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

#### real

Format: **r**eal [*min real* *max real*]

#### scale

Format: **sca**le _**log**arithmic/**lin**ear_

#### select

Format: **sel**ect *product* [*product* ...]

#### show

Format: **sho**w _**tvch**annels/**av**eraged**_

#### x

Format: **x**

