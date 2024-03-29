# Tutorial 1
An introduction to the software, and understanding online calibration.

## Goals of the tutorial

In this tutorial, you will learn:

* how to use [`rpfitsfile_server`](../../src/apps/rpfitsfile_server/),
  [`nvis`](../../src/apps/nvis/) and [`nspd`](../../src/apps/nspd),
* how the three applications interact with each other,
* what a typical 4cm CABB online calibration looks like,
* how CABB computes the data displayed in VIS and SPD,
* how CABB incorporates corrections made online,
* what some CABB settings do.

After completing this tutorial, you will be able to use the tools to learn more
in the other tutorials. You should also know what online calibration does, and
how to do it at a basic level.

Why do it this way? For several reasons:
* You can take your time to understand this stuff without having to learn it while
  you're trying to get high-quality science data.
* You can get a good understanding of what's expected before you use the
  telescope for the first time.
* You can manipulate the data in ways not possible while operating the telescope,
  but which will give you a better understanding of how the system works.

Let's get into it.

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

Let's begin by looking at the data with NSPD; this will give a basically unfiltered
view of the data that is coming from CABB. To do this, in one of the other terminals:
```bash
./nspd -d /xs -s 127.0.0.1 -u usr123
```

You should feel free to change `-d /xs` to use a different interactive PGPLOT device
if you'd like, and to change `-u usr123` to use a different username.

You should see a PGPLOT window open and look something like the image below. The
controlling terminal should have output something like:
```
 $ ./nspd -d /xs -s 127.0.0.1 -u usr123
client ID = Ij FH93YlB\R=Vame"H
Connected to SIMULATOR server.
NSPD> 
```

The terminal in which `rpfitsfile_server` is running will also update with more
information.

![NSPD display upon starting](nspd_t1_startup.png)

Finally, let's start up NVIS; this tool is used to see a more concise overview
of the data. To do this, in the last of the terminals:
```bash
./nvis -d /xs -s 127.0.0.1 -u usr123
```

Once again, you can change the PGPLOT device to whatever you want, but you should
make sure if you changed `-u usr123` for NSPD, do the same here for NVIS.

You should see another PGPLOT window open and look something like the image below.
The controlling terminal should have output something like:
```
 $ ./nvis -d /xs -s 127.0.0.1 -u usr123
Client ID = (Plwy@7=>RJw[c5{8BG
Connected to SIMULATOR server.
NVIS> 
```

Again, the `rpfitsfile_server` terminal should show some more output.

![NVIS display upon starting](nvis_t1_startup.png)

## Some basic usage

This data was taken during an online calibration process at the standard
4cm frequencies of 5500 and 9000 MHz. This can be most easily seen in NVIS.

The image below shows the same NVIS view as above, but annotated with the
times that each correlator calibration command was given; these times and
commands are:
Time       | Command
----       | -------
22:22:25   | dcal
22:24:02   | dcal
22:25:43   | pcal
22:27:23   | acal

![NVIS display annotated](nvis_t1_cal_labelled.png)

This is pretty typical, so it will be instructive to take a look at how
the data changes after each command. To do this, we'll use NSPD.

At the top of NSPD is a small panel of information, including the timestamp
for the data being displayed, the name of the source being observed (in this
case "1934-638"), the weather conditions during the cycle, the antennas
which are on-source, and the system temperatures for each antenna, polarisation
and continuum IF.

When NSPD started, it will probably have shown you the data from 22:21:14,
which is actually the midpoint of the cycle that started at 22:21:10. The cycle
time for this observation was 10 seconds. So why is 22:21:14 the midpoint? That
is just a function of the way that NSPD displays the time; the actual midpoint
is 22:21:14.95 (because of a small period of blanking that happens during the
cycle), and NSPD doesn't round but rather just truncates the seconds.

To begin, ensure that NSPD is displaying the data from 22:21:14. If it isn't,
you can tell it to by giving the command:
```
get time 22:21:14
```

Your display should look like the NSPD image above. You can see 25 panels
in a 5x5 grid, with each panel showing the correlated amplitude for a different
baseline. Let's run through what we're seeing. In the top-left panel, we can
see that the title is "AMPL.: FQ:1 BSL11". This indicates that we are seeing
the amplitude from the first IF for the baseline formed by antenna 1 with antenna
1; this indicates that it's an autocorrelation (sort of). The X-axis goes from
around 4500 to 6500; this is frequency in MHz. The Y-axis is amplitude, which
in this case has units of what we call Pseudo-Jy; we'll explain this in a later tutorial.

In this panel we can see four lines of different colour, and the colour key is
at the bottom right of the panel, under the X-axis. These lines are labelled:
Label    | Meaning
-----    | -------
AA       | the auto-correlation of the X-pol while the noise diode is off
aa       | the auto-correlation of the X-pol while the noise diode is on
BB       | the auto-correlation of the Y-pol while the noise diode is off
bb       | the auto-correlation of the Y-pol while the noise diode is on

Because the noise-diode is on for aa, you will see that its line looks almost
exactly the same as for AA, but at slightly higher amplitude (the same for bb and BB).
We'll discuss the operation of the noise diode later.

The panels progress from left to right and then downwards. There are six panels
containing the autocorrelations from the six individual ATCA antennas. The
seventh panel has the title "AMPL.: FQ:1 BSL12", and only has two lines. The X-axis
and Y-axis are exactly the same as for the autocorrelations, although the Y-axis range
is significantly different (and lower). This panel is a cross-correlation between
antenna 1 and antenna 2. The lines are labelled:
Label    | Meaning
-----    | -------
AA       | the correlation of X-pol on the first antenna with X-pol on the second antenna
BB       | the correlation of Y-pol on the first antenna with Y-pol on the second antenna

There are fifteen cross-correlation panels, all of which look similar but none
of them identical to any other. Throughout these tutorials, we will go through
why these panels look the way they do, learning about the how the correlator works
and how to judge the health of the data along the way. Here's a few questions to
keep in mind as you go along; if you already know all the answers to these, you
probably don't need to keep doing these tutorials! (Actually, I hope these tutorials
will teach everybody something, but maybe not the early ones...)

* What's with the slope of the lines?
* Why do the lines drop off at the edges?
* Why are the Y-axis scales not identical?
* Why does the amplitude not match the flux density of 1934-638?

After the six autocorrelation panels, and the fifteen cross-correlation panels from
IF1, we get another four autocorrelation panels, this time from IF2; the X-axis
for IF2 has a different frequency range.

You can play around with how many panels are shown on a single plot with the
`nxy` command in NSPD. This works similarly to the way it works in SPD, although SPD
has a usable limit of 4x4, whereas NSPD's limit is 7x7. For example, you can show all
twenty-one of the products from IF1 only with something like nxy 3 7; that will look
like the image below.

![NSPD display with `nxy 3 7`](nspd_t1_nxy37.png)

But first let's compare what we see on NSPD with what NVIS shows. To highlight the
time that NSPD is displaying on NVIS, give NVIS the command `data 22:21:14`. You
should see a vertical blue dashed line appear on the left side of the plot, with
some purple cross-hatching to the left of it. Don't worry about the cross-hatching
for now, as we'll discuss that later.

At the time of the blue dashed line, all the amplitudes on NVIS are very small
(< 0.5 PS-Jy). But on NSPD all the amplitudes are much larger than this. This is
the case for all the "products" that NVIS displays, which you can run through
with the commands:
Command    | Products Displayed
-------    | ------------------
`sel aa`   | the cross-correlation of X-pol on the first antenna with the X-pol on the second antenna, in the first IF
`sel bb`   | the cross-correlation of Y-pol on the first antenna with the Y-pol on the second antenna, in the first IF
`sel cc`   | the cross-correlation of X-pol on the first antenna with the X-pol on the second antenna, in the second IF
`sel dd`   | the cross-correlation of Y-pol on the first antenna with the Y-pol on the second antenna, in the second IF
`sel ab`   | the cross-correlation of X-pol with the Y-pol of the same antenna, in the first IF
`sel cd`   | the cross-correlation of X-pol with the Y-pol of the same antenna, in the second IF

For each selection, the key between colour and product is shown at the bottom of the plot,
where each product is labelled something like `12AA`, where the first antenna is the first
digit, the second antenna is the second digit, and the polarisation letters should match
the table above (although capitalised). NVIS currently only shows two IFs at a time (like VIS),
and the currently selected IFs are shown at the top right of the plot, along with their
associated polarisation codes and central frequencies in MHz. These can be changed, and we'll
discuss that in another tutorial.

To make NSPD display the products called "AB" and "CD" in NVIS, you can include `ab` in
the NSPD select command: `sel aa bb ab`. For each of the autocorrelation panels, you will
see a blue line at the bottom. This is the correlated amplitude of the noise-diode signal.
Because the noise diode is designed to be roughly 5% of the system temperature of the
receiver, it can be difficult to gauge the amplitude on the same scale as the actual
autocorrelated signal. To examine the amplitude of the noise diode more closely, you
can either change the amplitude scaling (for all the panels) with `a 0 50`, or you can
select only the AB polarisation with `sel ab`. To reset the amplitude scaling, simply type
`a`, and to reset the selection, just give another `sel` command like `sel aa bb ab`.

Anyway, as you ran through the products in NVIS, you will have noticed that at 22:21:14, 
all the amplitudes are very small, and much less than are displayed on NSPD. So why is this?
You may notice that there are two vertical yellow dashed lines on every NSPD panel; these
lines show the range "tvchannels", with all the channels between the two lines being
included. The amplitude parameter shown in NVIS is a vector average of all the complex
data in the tvchannels. That is, the NVIS amplitude depends not only on the amplitude
of the raw data, but also its phase. Note also that I didn't say that the amplitude parameter
was the mean of all the data, but the average; we'll discuss this in more detail in another
tutorial.

Let's look at the phase in NSPD then, to see why it is causing the NVIS amplitudes to be
so low. To do this, give NSPD the command `p`. Let's also ensure you're looking at the phase
of the noise diode, with `sel aa bb ab`. The plot you see in NSPD now should look like the
image below.

![NSPD displaying a rapidly wrapping phase](nspd_t1_phasewrapping.png)

In this mode, the Y-axis is phase in degrees, which can go between -180 degrees to 
+180 degrees. In each panel we can see varying rates of phase wrapping. For example,
the white AA line in the panel for baseline 1-4 is wrapping quite slowly, and the
phase does not fully wrap within the tvchannel range. The AA line for baseline 3-4
is wrapping slightly more quickly, and in the opposite direction to that seen in
baseline 1-4. And on baseline 1-6, it appears that the phase is wrapping so quickly
that it's difficult to discern any pattern.

We can also see that in NVIS while displaying the AA products, the line labelled
as "14AA" has the highest amplitude, followed by the line labelled by "34AA". This is
what we would expect for vector averaging; if the vectors (on the real-imaginary plane)
do not point in completely opposite directions, then there will be some bias towards
some direction, resulting in non-zero magnitude.

So is this the way it should look? Good question!

## Online calibration

To determine whether the data we're looking at is good, we have to first work out
what we expect it to look like. For this particular observation, the telescope is
looking at the source 1934-638, which is (for ATCA resolutions anyway) a point source
with a well defined flux density behaviour with respect to frequency, and a 
well-determined position.

Thinking about phase first, because it's a point source, we expect a plane wave to
hit all the antennas, with a delay between the wave hitting one antenna and another
determined entirely by the geometry of the array. Second, because we know its position,
we can direct our interferometer fringe patterns directly at the source, so that the
phase of the signal should be independent of baseline length, and thus independent of
frequency also. Thirdly, because it is a continuum source, it is generating waves at
all frequencies at the same time.

A simpler way of saying this is that we expect a continuum point source at the phase centre
to produce a frequency-independent signal with a single phase. This is definitely not
what we're seeing in NSPD at 22:21:14. Why not?

Looking closely at the behaviour of the phase, you can see that the phase appears
to vary linearly with frequency, and the slope of that line is degrees per MHz, or
seconds. This slope represents the combined delay error for each antenna and polarisation,
causing the wavefronts to be slightly misaligned at the correlator. These delay errors
are not due significantly to any geometry uncertainties, but rather to differences
in cable length, and timing differences between all the digitisers.

Back to NVIS now, we have panels for phase and delay. Phase is computed in the same
way as for amplitude, displaying the average value of the phase in the tvchannels
range. The delay (which as we've just discussed would be better labelled as
"delay error") is computed as the average value of the delay errors inferred by
looking at the phase changes between each pair of adjacent channels within the
tvchannels range. At 22:21:14, we
can again see the correspondence between what we see in NVIS and NSPD. For example,
the four baselines with the largest positive delay errors are 3-6, 4-6, 1-6 and
2-6, and they do appear to be wrapping far more often on NSPD than the other baselines.
Then we can see that the baseline 1-2 has only a small positive delay error, while the
baseline 1-3 has a similarly small negative delay error, and this is reflected in NSPD
where the phase is increasing with increasing frequency for baseline 1-2, and 
decreasing for baseline 1-3. You should run through all the IF 1 products available
in NVIS ("aa", "bb" and "ab") and check that you understand the correspondence between
NVIS and NSPD.

Looking at NVIS, we can see that something happens at 22:22:45, where the delay
errors go to 0, the amplitudes all increase and the phase changes. Let's look at
what the data looks like shortly after that time in NSPD, by giving it the command
`get time 22:23`. You might see something like the picture below (if you've
set NSPD up with the commands `sel aa bb ab`, `sel f1` and `p`).

![NSPD display after the first dcal](nspd_t1_phaseafterdcal.png)

Now the phase is not wrapping on any of the baselines, but it still has a slope
on some, while it's "flat" on most of them. Let's discuss what has happened here,
since it will give us a lot of insight as to how CABB works.
As stated in the table shown earlier in this tutorial, a "dcal" command was given at
22:22:25. At that time, CABB was accumulating data for the cycle between
22:22:20 and 22:22:30, and processing the data between 22:22:10 and 22:22:20.
However, CABB has already prepared the fringe rotation model for the next cycle
which will run between 22:22:30 and 22:22:40. Thus the first cycle that can
incorporate the new delay figures will run between 22:22:40 and 22:22:50, and
that's what we see in NVIS. You should remember this: if you give CABB a "dcal"
or a "pcal" command, the effect will not be seen in the next cycle, but only in
the cycle after that.

The top two rows of panels show a light blue phase line, and it looks very bumpy on
a couple of the antennas. This is the correlated phase of the noise diode between
the two receptors on each antenna. The noise diode is injected in between the
two receptors, but each receptor will respond to the signal slightly differently, and
thus the correlated amplitude and phase is not single-valued over the whole frequency
range. The same is true for the correlations between antennas. But the primary reason why
some antennas look bumpier than others in the NSPD plot is because of the narrower
Y-axis range on some panels. It is therefore sometimes more useful to plot only the
normal phase range so comparison between panels is easier, with `p -180 180`.

We can see then that the phase slope on the products involving antenna 3, and that
for the cross-correlations the slope is on the Y-pol but not the X-pol. Again,
this gives some insight to how CABB works. When the dcal command was given at
22:22:25, CABB calculated the delay corrections to be:
```
01-16 22:22:25:: CCOMMAND: 'dcal'
01-16 22:22:25:: DCAL 1A: Corrections: CA01=-2.02 CA02=-4.64 CA03=0.00 CA04=-1.32 CA05=-31.80 CA06=-54.22 nS
01-16 22:22:25:: DCAL 1B: Corrections: CA01=-15.91 CA02=-7.55 CA03=0.00 CA04=-4.37 CA05=-42.71 CA06=-111.05 nS
01-16 22:22:25:: DCAL 1AB:Corrections: CA01=7.08 CA02=-3.90 CA03(Ref)=-6.82 CA04=-3.76 CA05=4.10 CA06=50.02 nS
01-16 22:22:25:: DCAL 2A: Corrections: CA01=-13.26 CA02=30.98 CA03=0.00 CA04=-66.97 CA05=-31.37 CA06=-73.56 nS
01-16 22:22:25:: DCAL 2B: Corrections: CA01=-35.57 CA02=51.68 CA03=0.00 CA04=2.45 CA05=-77.86 CA06=-40.91 nS
01-16 22:22:25:: DCAL 2AB:Corrections: CA01=37.71 CA02=-5.31 CA03(Ref)=15.40 CA04=-54.52 CA05=61.87 CA06=-17.25 nS
```

As you can see, no corrections were made to CA03 for 1A, 1B, 2A or 2B. This is because
it was the "reference" antenna, and all delays are with respect to this reference. However,
there is also a delay between the two receptors on each antenna, which is measured by
examining the noise diode delay, and assuming that the noise diode signal should hit
the two receptors simultaneously. But again, we need a reference, and this time we
have a reference receptor, that being the X-pol receptor, and so we add delay to the
Y-pol receptor's signal. So the correlator is over-correcting the Y-pol of the reference
antenna. Why? It is a known bug, which only seems to occur in the first dcal after the delays 
have been reset to their defaults (with the "reset delays" correlator command).

You can see this correspondence between the Y-pol over-correction and the cross-correlation
delays in NVIS. Give it the commands: `hist 6m10s` and `sca d -0.3 0.3`. Then
swap between `sel bb` and `sel ab` to see how closely the delay in 33AB matches that for
13BB and 23BB. The delays for 34BB, 35BB and 36BB have the same magnitude but opposite
sign; why do you think that is?

Another thing you might notice on the NSPD display is the odd look of the 2-6 and 5-6 baseline
panels, where there are many vertical white strips of colour. This occurs because of a
different type of phase wrapping. In this case, the phase of the X-pol signals on these
baselines sits very close to -180 degrees. The natural bumpiness of this phase signal
over the frequency range sometimes makes the phase go to less than -180 degrees, which
is equivalent to the same phase + 360. Because we calculate the phase between -180 and 180
degrees, the phase appears to jump between the two extremes, causing the NSPD appearance,
even though the phase signal is no different to any of the other baselines.

Once again, you should be able to see the correspondence between the average phase
in the tvchannel range as displayed on NVIS with the data displayed in NSPD.

This remaining slope in phase can be corrected with another dcal, which is what
happened at 22:24:02, with the corrections:
```
01-16 22:24:02:: CCOMMAND: 'dcal'
01-16 22:24:02:: DCAL 1A: Corrections: CA01=-0.00 CA02=0.00 CA03=0.00 CA04=-0.00 CA05=-0.00 CA06=-0.00 nS
01-16 22:24:02:: DCAL 1B: Corrections: CA01=-0.24 CA02=-0.24 CA03=0.00 CA04=-0.24 CA05=-0.25 CA06=-0.25 nS
01-16 22:24:02:: DCAL 1AB:Corrections: CA01=0.01 CA02=0.01 CA03(Ref)=-0.24 CA04=0.00 CA05=0.01 CA06=0.00 nS
01-16 22:24:02:: DCAL 2A: Corrections: CA01=0.00 CA02=0.00 CA03=0.00 CA04=-0.51 CA05=-0.00 CA06=-0.00 nS
01-16 22:24:02:: DCAL 2B: Corrections: CA01=-0.24 CA02=-0.24 CA03=0.00 CA04=-0.24 CA05=-0.25 CA06=-0.25 nS
01-16 22:24:02:: DCAL 2AB:Corrections: CA01=-0.00 CA02=-0.01 CA03(Ref)=-0.24 CA04=-0.50 CA05=-0.01 CA06=-0.00 nS
```
The corrections were thus incorporated into the cycle that started at 22:24:20, which
you should now view in NSPD to confirm that the slope has gone away.

Once the phase is "flat" across the band, the next step is often to correct any
phase offsets. As you can see in NSPD, there are some baselines where the phases
of the "AA" and "BB" correlations lie close to each other, and other baselines where
they are separated. And the average phase of each line (as you can see from NVIS)
is not always very close to 0. Recall that we are observing a point source at the
phase centre, so the observed phase *should* be 0. But phase is a relative term, as it
certainly doesn't relate to a zero-crossing of the sine wave that is the voltage
at some frequency induced by the incoming radiation. So we're perfectly entitled to
introduce phase offsets into our signal to assign the phase we're seeing the label
"0". That's what the pcal command does, and at 22:25:43, that command was given.
The correlator output the following at that time:
```
01-16 22:25:43:: CCOMMAND: 'pcal'
01-16 22:25:43:: PCAL 1A: Corrections: CA01=83.97 CA02=76.85 CA03=0.00 CA04=80.99 CA05=69.43 CA06=-112.84 degs
01-16 22:25:43:: PCAL 1B: Corrections: CA01=-3.96 CA02=-13.29 CA03=0.00 CA04=-7.80 CA05=-20.49 CA06=67.52 degs
01-16 22:25:43:: PCAL 1AB:Corrections: CA01=-1.29 CA02=2.59 CA03(Ref)=-89.60 CA04=0.26 CA05=2.26 CA06=89.31 degs
01-16 22:25:43:: PCAL 2A: Corrections: CA01=80.86 CA02=67.18 CA03=0.00 CA04=74.89 CA05=54.10 CA06=-63.06 degs
01-16 22:25:43:: PCAL 2B: Corrections: CA01=-6.31 CA02=0.64 CA03=0.00 CA04=-12.42 CA05=-32.79 CA06=116.66 degs
01-16 22:25:43:: PCAL 2AB:Corrections: CA01=-4.04 CA02=-24.30 CA03(Ref)=-90.72 CA04=-0.66 CA05=-2.15 CA06=91.43 degs
```

Again, CA03 is the reference, and the offsets for each antenna are equivalent
to the phase observed on the baselines between that antenna and CA03 (and you can
confirm this on NVIS, and you will again see that the phase corrections to CA04,
CA05 and CA06 are opposite in sign to the phase observed on those baselines).

This phase offset is incorporated into the fringe rotation just like the delay
model is, so again, the data only reflects the offset two cycles later, at
22:26:00. If you view this time in NSPD, you will see something like the picture
below.

![NSPD display after the pcal](nspd_t1_phaseafterpcal.png)

At this point the telescope's phase is completely calibrated. We'll discuss this
more later, but it is important that you check all the products that NSPD displays
during a normal observation, including not only the cross-correlations, but the
autocorrelations as well.

The final step in a normal online calibration is to set the flux density scale.
The flux density of 1934-638 is 4.965 Jy at 5500 MHz, which is clearly not
what NVIS is showing (about 2.5 Jy). To set the proper flux density scale,
give the acal command. **Note:** acal can take some optional arguments, and
we'll discuss them later. For now, because 1934-638's flux density doesn't change
over time, its model is built into caobs, and caobs will handle telling the
correlator the correct flux density when you give the acal command. In this
observation, the acal was given at 22:27:23. The correlator output from that time was:
```
01-16 22:27:23:: CCOMMAND: 'acal  4.965  2.701'
01-16 22:27:23:: ACAL 1A: NCal (Jy): CA01=23.83 CA02=19.22 CA03=20.82 CA04=20.25 CA05=17.84 CA06=23.03
01-16 22:27:23:: ACAL 1B: NCal (Jy): CA01=25.20 CA02=21.40 CA03=19.37 CA04=19.80 CA05=16.74 CA06=22.84
01-16 22:27:23:: ACAL 2A: NCal (Jy): CA01=38.45 CA02=35.21 CA03=33.02 CA04=33.88 CA05=28.29 CA06=33.35
01-16 22:27:23:: ACAL 2B: NCal (Jy): CA01=41.74 CA02=32.90 CA03=30.52 CA04=37.63 CA05=25.43 CA06=34.77
```

The correlator measures the effect of the noise diode every cycle, and the acal
command allows the correlator to associate this effect with a known flux
density. We'll go into how it does this in another tutorial. For now though,
we can see that for each antenna, the flux density of the noise diode is similar,
although not exactly the same for each of the two polarisations at each frequency,
but it differs significantly for the two IFs.

You will also notice that the amplitude in NVIS changes in the cycle that
goes from 22:27:20 to 22:27:30, i.e. the cycle during which the acal command
was given. This is very different to the dcal and pcal commands which only
show their effect two cycles after they were given. Why is this? The amplitude
is simply the magnitude of the complex correlation coefficient, which can be
scaled without issue while preserving the phase information. So the multiplication
scaling factor can be applied at the end of the correlation and integration process,
whereas the delay and phase corrections are more naturally performed during the
fringe rotation stage during the integration. We will get into much more depth
about how this works in a future tutorial, because it is important to understand
what effects this process has on your data.

The last thing to notice now is what happens to the system temperatures when
the acal command is given. At 22:27:14, the system temperatures (as shown at the
top of NSPD) are (in Kelvin):
```
TSYS      CA01          CA02          CA03          CA04          CA05          CA06
IF1    13.4 / 12.7   14.0 / 13.2   14.0 / 14.3   14.2 / 14.3   14.8 / 15.2   13.3 / 13.0
IF2     9.2 /  8.8    9.4 /  9.5    9.4 / 10.2    9.7 /  9.4   10.7 / 11.2   10.0 /  9.7
```

At 22:27:24, after the acal, they are much more realistic:
```
TSYS      CA01          CA02          CA03          CA04          CA05          CA06
IF1    20.8 / 20.3   19.4 / 19.4   20.3 / 19.8   20.1 / 20.2   20.2 / 19.8   20.2 / 19.6
IF2    18.1 / 18.0   17.5 / 17.3   17.8 / 17.7   17.9 / 18.2   17.7 / 17.9   18.2 / 18.1
```

You can also see this in NVIS, which can show you both the system temperature and the
assumed flux density of the noise diode, by showing the `n` (noise diode flux density)
and `S` (system temperature) panels; try giving the NVIS command `apdnS-t` to see for
yourself (or look at the image below). This is a feature that is not available in VIS.

![NVIS display with apdnS-t](nvis_t1_apdnSt.png)

## What we've covered so far

Before we go on, let's take a look at what we've covered so far.

* You should now be able to start the rpfitsfile_server, and connect to it with
  NSPD and NVIS.
* You know a few commands in both 
  [NSPD](../../src/apps/nspd/) and [NVIS](../../src/apps/nvis/); remember that 
  the documentation pages for these tools have the complete list.
* You've seen what the data looks like before the online calibration process
  starts, after dcal, after pcal and after acal.
* You know how CABB takes the data that appears in NSPD and computes the
  amplitude, phase and delay values that appear in NVIS. You can also identify
  correspondences between the two displays.
* You know how CABB incorporates the correction factors for delay, phase and
  amplitude.

But we've so far focussed on IF 1, with a central frequency of 5500 MHz, and
pretty much completely ignored IF 2, at 9000 MHz. Let's remind ourselves that these
data represent a successful online calibration process for both IFs, and take a look
at IF 2 to learn a few more interesting things.

## The second frequency

Lets go back to the time after the first dcal in NSPD, and then select IF 2, with
the commands `get time 22:22:45`, `sel f2`, `p -180 180`. It should look like the
image below.

![NSPD IF2 after the first dcal](nspd_t1_f2_afterdcal.png)

As you can see, like in IF 1, there is a residual slope in phase, although now
it's on both CA03 and CA04, and in the X-pol as well as the Y-pol. Let's see
what NVIS looks like, with the commands `hist 6m10s`, `sel cc`; it should look
like the image below.

![NVIS showing IF 2 X-pol after the first dcal](nvis_t1_cc.png)

It looks quite different to `sel bb` does, especially for the delay panel. For
IF 1, the delays do look noisy, but they're not jumping around every cycle
like they seem to be doing for IF 2.

Before we go any further, let's revisit how the correlator performs a dcal. When
the dcal command is given, the correlator calculates the appropriate delay
corrections to make and outputs the corrections for the user to see; there are
some examples of this output in the previous section. But how does it calculate
these delay corrections? It uses the same data that it sends to VIS, that being
the averaged data in the tvchannel ranges. But it doesn't just rely on the most
recent cycle to calculate these corrections, it takes the average value from the
most recent `nncal` cycles, so as to minimise noise. For 99% of ATCA observations
`nncal` is set to 3.

Since the dcal command is given at 22:24:02, the most recent cycle that the
correlator has finished has data from between 22:23:40 and 22:23:50, since
the cycle that finished at 22:24:00 is still being processed. The three
cycles used to compute the delay corrections would thus have midpoints of
22:23:25, 22:23:35 and 22:23:45. From NVIS we can see that some of the baselines
had +/- 1 ns jumps in their delay error values in those cycles. And yet, we know
that the dcal was successful, so the correlator knew the right corrections to
make. 

This brings us back to something you should not forget: what is displayed
in VIS or NVIS **is not the data**. The data (as seen in SPD or NSPD) can be
perfectly good, or very bad, and it is possible in either case to make the
VIS display look perfectly reasonable or horrible just by changing some
correlator parameters. Before any correlator calibration command (dcal, pcal
or acal), you need to ensure that VIS displays realistic and stable values
for whichever thing you're calibrating (delay, phase or amplitude) for at least
the most recent 3 cycles. Failure to do this will often result in the calibration
command making the situation worse. This tutorial, and the next couple will
go through all the ways that you can change correlator parameters to produce
realistic and stable values (and ways in which changing correlator parameters
can produce unrealistic values too, just so you can avoid doing that).

In this particular case, the correlator parameter we need to change is the
tvchannels. It is probably fairly obvious that at frequencies between 9300 and
9400 MHz, the phase looks fairly messy on NSPD. If you switch to looking at
amplitude, you will see some pretty strong RFI, in this case caused by transmission
from satellites. We can tell that these satellites are fairly close by because
the signal is stronger on shorter baselines (or weaker on baselines to CA06). That
is, antenna 6 is not looking directly at the same satellite even though it's pointing at
the same azimuth and elevation as the rest of the antennas a few km away.

This RFI is within the tvchannels range (as illustrated in NSPD by being between
the two yellow dashed lines). You can also find out the tvchannels range by
giving the command `print comp` in NVIS, which will output something like:
```
NVIS> print comp
VIS DATA COMPUTED WITH OPTIONS:
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
```

Here you can see that for both of the IFs (called WINDOWS in the output),
the tvchannel range is set to cover the channels between 513 and 1537.
While observing for real, the current tvchannel range settings are displayed 
near the top of the CACOR window.

To switch NSPD over to display channels instead of frequency, give it the
command `x`. The X-axis should change to display 0 on the left and about 2000
on the right (the same command again toggles the X-axis back to frequency).

Determine a channel range that excludes the RFI. You can zoom in to a narrower
range of channels in NSPD using the `chan` command. For example, `chan f2 1000 1500`
will display only the channels between 1000 and 1500 for IF 2 (`chan` by itself
will reset NSPD to display all the channels again). When you've found a channel
range that is free of RFI, change the tvchannel range in NVIS by giving the
command `tvchan f2 low high`, where `low` and `high` should be replaced by the
channel numbers of the lowest channel to include and the highest respectively.
For example, you might choose the range `tvchan f2 513 1300`. After the
server recomputes the data, you will see the yellow dashed lines move to reflect
the change in NSPD, and you might see something like the following image in
NVIS.

![NVIS display with `tvchan f2 513 1300`](nvis_t1_f2_513_1300.png)

It is important to note at this point that the way this software operates is
very different to the way real observations work. When you change the tvchannels
while doing real observing, only data in the future is affected; all previous
data displayed in VIS reflects what the tvchannels were when that data was
obtained. In this software, all NVIS data is recomputed to show the effect
that changing the tvchannels has. This allows you to see quite clearly the
effect of doing something, so that you can understand what is happening.

It is also worth reminding you here again that when you changed the tvchannel
range, you did not affect the actual data; only the summary data displayed
in NVIS changed. (OK, this is not entirely true, but we'll get to that in
another tutorial. For now, it's a harmless enough untruth.)

Looking at NVIS now, we can see that the data displayed in the delay panel
looks much more ordered, very similar to what we saw for IF 1. For reference,
during the observation the tvchannels in IF 2 were set to be 200 - 1149. 
Does NVIS look any different when set to this range instead of what you chose? 
The delays and phases probably don't, but the amplitude might.

## Summary

We've covered a lot of stuff in this tutorial. Let's quickly summarise what
this tutorial should have taught you to do during the online calibration
process.

1. Look at SPD to assess the state of the amplitudes and phases in all your
   frequency bands.
2. Determine the appropriate range of channels to use as tvchannels to avoid
   RFI.
3. If there is a significant slope in phase across your bands, ensure that
   VIS shows a delay error which is stable with time. Wait until the 3 most
   recent cycles show effectively the same delay error (in all the products
   that VIS displays), and then do a dcal.
4. Repeat steps 2 & 3 if necessary until NSPD shows no significant phase slopes.
5. Wait until at least 3 cycles after a dcal, and then do a pcal.
6. Wait until at least 3 cycles after a dcal, and then do an acal (if you're
   looking at 1934-638).
7. Look at SPD to ensure the phases and amplitudes look the way you expect for
   a properly calibrated array.

You should also know:
* How CABB takes the data that appears in NSPD and computes the
  amplitude, phase and delay values that appear in NVIS. 
* How to identify correspondences between NVIS and NSPD.
* You know how CABB incorporates the correction factors for delay, phase and
  amplitude, and how long it takes for data to be affected by changes to the
  correction factors.
* The relation between AA, BB, CC, DD, AB, CD products and the IFs and the actual
  X- and Y-polarisations from the antennas.
* That the antennas have noise diodes, and what effect they have on the
  autocorrelations in SPD, and in the AB and CD products in VIS.
* That the measured system temperatures rely on the noise diodes and the acal
  command.
* That SPD shows the only true representation of the data, and that VIS can
  be manipulated to show an inaccurate overview.
* Why we think that understanding how the correlator works is an important
  part of using the telescope effectively.

When you're comfortable with all the material in this tutorial (and this will
definitely be the longest of the tutorials), progress to the
next tutorial. We'll cover the dcal process in a bit more detail, and talk about
the "delavg" correlator parameter.

Here's a few questions and challenges to see how well you understand the system.

* Why do we need a reference antenna? (EASY)
* Read the documentation of [NVIS](../../src/apps/nvis/) and play around with
  all the available panels. (EASY)
* Using ONLY things covered in this tutorial, make NVIS look like the following
  image, where the amplitude doesn't change before and after the two dcals. 
  (DA-LEVEL CHALLENGING)

![NVIS challenge](nvis_t1_challenge.png)

