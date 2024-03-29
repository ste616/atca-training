# Tutorial 3
A look at amplitude calibration, how it works, and its effect on
system temperatures.

## Goals of the tutorial

In this tutorial you will learn how amplitude calibration works in CABB,
and how system temperatures are set through this process. You will learn
how to do this calibration as correctly and consistently as possible,
and how best to avoid some of the potential pitfalls.

## Starting the tools for this tutorial

It is assumed that you have already completed both [tutorial 1](../1_introduction/)
and [tutorial 2](../2_dcal_and_delavg/), and so have already obtained and
compiled the tools.

Once again, we open up three terminals. In one of these terminals, navigate to
the path `atca-training/tutorials/2_dcal_and_delavg` and in the other two
go to `atca-training/build`.

Since you have already completed [tutorial 2](../2_dcal_and_delavg/), you should
already have downloaded the RPFITS file that we will use for this tutorial. As
noted above, we will operate out of that tutorial's directory, so start
the `rpfitsfile_server` with the command:
```bash
../../build/rpfitsfile_server -n 2021-04-28_0445.C999
```

When you see the `Waiting for connections...` output, you can start using
the other tools.

Start NSPD in one of the other terminals:
```bash
./nspd -d /xs -s 127.0.0.1 -u usr123
```

You should feel free to change `-d /xs` to use a different interactive PGPLOT
device if you'd like, and to change `-u usr123` to use a different username.

Start NVIS in the third terminal:
```bash
./nvis -d /xs -s 127.0.0.1 -u usr123
```

Once again, you can change the PGPLOT device to whatever you want, but you
should make sure if you changed `-u usr123` for NSPD, do the same here for NVIS.

At this point NSPD and NVIS should look something like the following two
images.

![NSPD upon startup](nspd_t3_startup.png)

![NVIS upon startup](nvis_t3_startup.png)

## The observation

In this dataset, two sources were observed. From roughly 04:46 to 04:48:15
we observed the source 0945-321 which has a flux density of about 323 mJy
at 5.5 GHz, and from about 04:49:30 until 04:52:10 we observed 0823-500,
which has a flux density of about 2.879 Jy at 5.5 GHz (a ~9 times higher
flux density than 0945-321).

## Initial look at the data

Immediately we can see that the flux densities of the two sources, as
they appear in NVIS, do not match those just quoted. After the previous
two tutorials, you should understand that at this point there is a lot of
decorrelation due to the way vector averaging works with data that is not
delay calibrated. So you should start by delay calibrating all the data, and
then you should see that NVIS looks something like the following:

![NVIS after a dcal](nvis_t3_after_dcal.png)

You should notice a few things about the amplitudes at this point.
First, they're clearly much higher than we think they should be. Second,
the amplitudes on each baseline don't agree with each other very well.
Third, it looks like the amplitude for 0945-321 increases at about 04:47:45.

We know that both sources are point-like, so the amplitudes should be
consistent across all the baselines. We also know that they are unlikely
to change their flux densities suddenly. So why does NVIS appear the way it
does? Figuring that out will reveal more details about how CABB works, and
why. Having this knowledge should help you when you are observing or doing
data reduction.

## CABB's amplitude correction model

Let's begin by describing what's going on in the correlator, in a
less-technical way than we will later.

The data coming from each antenna into the correlator is simply a stream
of voltages over time. When each antenna is pointing at the same patch
of sky, one might expect that this stream would look very similar on each
antenna, with random noise variations being the main difference, but this
is not necessarily the case. Even in a homogenous array like ATCA, the
equipment on each antenna is not really identical, and that's before we
start to consider controllable attenuators and differences in age and
the time since different parts were tuned or replaced.

When the time comes for the correlator to multiply the data from one
antenna with another, it cannot be assumed that the individual antennas
share the same voltage scaling. So what comes out of the multiplication?

Let's think about what we want the result to be. Imagine an empty patch
of sky, except for a point source with some known flux density at a
well-defined position. We've told our correlator to correct for delays
and phases towards that point source. In this case, we want each channel
of our cross-correlation to represent the flux density of the point
source in that frequency channel, with a phase of 0 because it's positioned
at the phase centre. But for that to be true, both antennas have to agree
on the flux density that they are looking at, and this is the aim of
amplitude calibration.

So how do we ensure this to be the case? Like a lot of things in radio astronomy,
it comes down to comparisons with known standards. We begin by pointing
at a source with some known flux density, and calibrate our antennas
against this. Then, when we observe some other source with an unknown
flux density, we simply assume that the system has a linear response to
flux density such that if a source had twice the amount of flux density,
the system would produce twice the response compared to the standard.

Sounds easy, and it is, so long as the system stays perfectly linear between your
observation of the flux density standard, and your observation of the
target. However, that is rarely the case, because the response of the
system depends on so many things: the atmosphere between the telescope
and the source, effects of pointing elevation on the reflector shape,
shifts in antenna focus, etc.

We can group most of these types of changes into a single parameter though:
system temperature. The basic idea is that an instrument with a lower
system temperature is more sensitive, and thus will produce a larger response
to the same input. If we keep track of the system temperature of the
instrument, we can compensate for the non-linearity caused by changes in
sensitivity, and recover the linear response we require for precision
measurements.

This is best expressed as the "system equivalent flux density", which is
the flux density of a source which would double the response of a system.
Imagine that when you look at a completely empty patch of sky, your
instrument produces an output of *A*, but when you observe a source with a
flux density of *S* Jy, your instrument produces an output of *2A*; in
this case, your system equivalent flux density (SEFD) is *S* Jy. You can
see then, that if for some reason your SEFD increased to *2S* Jy, that same
source would only increase the output by 50%, instead of the 100% increase
it generated while your SEFD was *S* Jy.

Ideally then, we want to continually assess what our SEFD is during
our observations. We could repeatedly go back and observe the same
standard flux density calibration source, but that adds a great deal of
overhead. What would be better is if we could carry around a source with
known flux density, and continually observe how much the system responds
to that source, even while we're looking at other sources. And, for the
ATCA, we do have such a system: the noise diode.

On each antenna, for all bands except for 3mm, we have installed a
broadband noise source which is engineered to be very stable over the
typical timescales for observing. This noise source is switched on and
off 8 times per second, and the correlator monitors the data to continually
assess the effect the noise diode has, and compute the current system
temperature for each cycle. The correlator then scales the data
by the computed system temperature, to keep the system linear.
Thus it is important to calibrate the
amplitude of the noise diodes, so that the monitoring is as accurate as
possible; this is what the amplitude calibration process does.

## A look at the data

Let's take a closer look at the data in NSPD. Configure it to view only
the auto-correlations for both IFs, view 0823-500, and set an amplitude
scale which keeps all the data in view. The following commands will do
that, but feel free to go with your own solution.

```
NSPD> off ccs
NSPD> get time 04:51
NSPD> nxy 3 4
NSPD> a 100 6500
```

You will see something that looks like the following image.

![NSPD view of 0823-500 auto-correlations](nspd_autos_0823-500.png)

This illustrates quite well the idea that each antenna is on a
different scale. Consider CA04, which has a much higher amplitude
than all the other antennas in both IFs. You might think this is
because the antenna is generating more noise than the other antennas, but
look more carefully at the red and white lines. The red line is the
power in the X polarisation while the noise diode is on, while the white
line is the power in that same polarisation while the noise diode is off.
Compare the size of the gap between these two lines for each antenna, and
you'll see that it is fractionally very similar on all the antennas. That
is, the noise diode on CA04 is pumping up the amplitude when it is on
by about the same ratio as for CA03; this can be made clearer by giving
NSPD the command `a` to let each panel use its own y-axis range.

You can see the effect of this different scaling clearly on NVIS, with
the baselines with the highest amplitudes on 0823-500 all containing
CA04. It might be useful now to discuss how amplitudes are determined
from the data.

We label the output from the correlation process the complex number
C<sub>ij</sub>, for antennas i and j. This is a power
(because it is a multiplication
of two numbers), and thus has units of something like Volts^2 (if the raw
input numbers to the correlator were in Volts). The amplitude is just the
magnitude of this complex number, but it is presented in NVIS as (pseudo-)Jy.
How do we do this translation? By multiplying by the square root of the
product of the system temperatures of the two antennas, where the
system temperature is expressed in the desired units (in this case Jy).
But we also need to take away the units of C<sub>ij</sub>, and we do this by
dividing it by the square root of the product of the average power
levels on each antenna. We get these values from the auto-correlation power
spectra. From NSPD you can see we have two power spectra per polarisation,
IF and antenna: one from when the noise diode is on, and the other when it
is off. Because the noise diode has a duty cycle of 50% (half on, half off),
the average power is just the mean of these two powers, and we call this the
"gated total power" (GTP).

So, the full equation for how the amplitude is determined is:

![A_{ij}=\frac{|C_{ij}|\sqrt{T^{sys}_{i}\times T^{sys}_{j}}}{\sqrt{GTP_{i}\times GTP_{j}}}](https://latex.codecogs.com/svg.latex?A_%7Bij%7D%3D%5Cfrac%7B%7CC_%7Bij%7D%7C%5Csqrt%7BT%5E%7Bsys%7D_%7Bi%7D%5Ctimes%20T%5E%7Bsys%7D_%7Bj%7D%7D%7D%7B%5Csqrt%7BGTP_%7Bi%7D%5Ctimes%20GTP_%7Bj%7D%7D%7D)

where A<sub>ij</sub> is the amplitude, T<sup>sys</sup><sub>i</sub> is
the system temperature of antenna i, and GTP<sub>i</sub> is the GTP
of antenna i. A<sub>ij</sub> will have the same units as T<sup>sys</sup><sub>i</sub>
so long as C<sub>ij</sub> has the same units as GTP<sub>i</sub>.

We measure GTP<sub>i</sub> directly from the data, but we need some way
to determine T<sup>sys</sup><sub>i</sub>. If we know the amplitude of
the noise diode, and the noise diode is switching on and off, there
is a pretty straightforward relationship: the system temperature will
be the amplitude of the noise diode multiplied by the inverse of the effect it has
to the output power caused by the noise diode. We have to use the inverse
here because a system with a higher system temperature will respond less to
the same input than a system with a lower system temperature.

The absolute change in the output power due to the noise diode is called the
"synchronously-demodulated output" (SDO), and
is simply defined by the difference in the power between the on and off
states. The fractional change is just this SDO divided by the average power,
which is the GTP. In other words, taking into account the inverse relationship:

![T^{sys}_{i} = N_{i}\frac{GTP_{i}}{SDO_{i}}](https://latex.codecogs.com/svg.latex?T%5E%7Bsys%7D_%7Bi%7D%20%3D%20N_%7Bi%7D%5Cfrac%7BGTP_%7Bi%7D%7D%7BSDO_%7Bi%7D%7D)

where N<sub>i</sub> is the amplitude of the noise diode on antenna i,
and SDO<sub>i</sub> is the SDO of antenna i.

You might notice that we can combine these two equations for simplicity,
since:

![\frac{T^{sys}_{i}}{GTP_{i}} = \frac{N_{i}}{SDO_{i}}](https://latex.codecogs.com/svg.latex?%5Cfrac%7BT%5E%7Bsys%7D_%7Bi%7D%7D%7BGTP_%7Bi%7D%7D%20%3D%20%5Cfrac%7BN_%7Bi%7D%7D%7BSDO_%7Bi%7D%7D)

we can change the amplitude equation to one which only requires a constant
and a single measure:

![A_{ij}=\frac{|C_{ij}|\sqrt{N_{i}\times N_{j}}}{\sqrt{SDO_{i}\times SDO_{j}}}](https://latex.codecogs.com/svg.latex?A_%7Bij%7D%3D%5Cfrac%7B%7CC_%7Bij%7D%7C%5Csqrt%7BN_%7Bi%7D%5Ctimes%20N_%7Bj%7D%7D%7D%7B%5Csqrt%7BSDO_%7Bi%7D%5Ctimes%20SDO_%7Bj%7D%7D%7D)

This is what the correlator does every cycle; it measures the SDO from the
autocorrelation data, and takes the known amplitude of the noise diode,
and modifies its output appropriately. Do you remember back in tutorial 1,
we saw that an acal command took immediate effect? This is why: the modification
here occurs after the data has been correlated and the SDO is measured, whereas delay
and phase shifts are accounted for during the correlation process.

And because the amplitude scaling is done in this way, it is completely
reversable, if we know the SDO and the amplitude of the noise diode that
was used by the correlator for each cycle. This data is indeed stored in
the data file, and we can view this, and system temperature and GTP, in NVIS
with the panels `O` (SDO), `n` (noise diode amplitude), `S` (system temperature)
and `G` (GTP). If you tell NVIS to view these panels along with the amplitude,
you will see something like the image below.

![NVIS showing amplitude calibration-related parameters](nvis_t3_aOnSG-t.png)

There is quite a lot of information in these plots, so let's touch on a few
of the more interesting points.
* The amplitude of the noise diode on CA04 is much higher than on the other
  antennas, and the system temperature is much higher as well, but the SDO
  and GTP of the antenna is pretty much in line with the others; yet another
  clue that the problem with this antenna is that the noise diode amplitude
  is incorrect.
* System temperature is given in Kelvin here, not Jy. There is a conversion
  between these two units, which we will go into later. System temperature
  is stored in the file as Kelvin, and this doesn't matter because only SDO
  and the noise diode amplitude are required for amplitude manipulation.
* When the amplitude of 0945-321 changes (at around 04:47:45), the SDO, GTP
  and system temperature all change in a step fashion, but the noise diode
  amplitude is not affected. In particular, the SDO decreases, so the
  computed source flux density must increase since it looks to the correlator
  like the source is now having a bigger effect on a less sensitive
  instrument. Why this occurred will be described later, because this is
  exactly what we would prefer didn't happen, and pretty much why this
  tutorial exists!
* There is data while the antennas are slewing between 0945-321 and 0823-500,
  and we see during this period that the system temperature and GTP are
  decreasing, while SDO stays constant. This is due to elevation effects:
  0823-500 was at a higher elevation (about 49 degrees) than 0945-321
  (about 32 degrees), and we can see the transition is pretty smooth as the
  elevation increases. We can also see from the data that the antennas
  must have reached their target elevation before they had finished slewing
  in azimuth.

## Computing SDO and GTP

You may have noticed also that the plot above shows very large numbers for
GTP and SDO, and may then be wondering about exactly how the correlator
determines these values. As we said before, these numbers come from the
raw data, before the scaling process occurs. So let's take a look at those
raw data, by giving NVIS the command `tsys off`. This causes the server
to reverse all the amplitude scaling, and display the raw data in both
NVIS and NSPD. Below is an image showing the raw data for the
auto-correlations in NSPD.

![Raw data auto-correlations in NSPD](nspd_t3_raw_data.png)

Now the amplitudes of the data match the GTP and SDO, and we can see where
these numbers come from; each of them is the average value within the
tvchannel range (and this average can be the mean or median, depending
on the **tvmedian** setting).

The NVIS display looks like this:

![Raw data in NVIS](nvis_t3_raw_data.png)

And here you can notice that the step change in GTP, SDO and system
temperature that was seen while observing 0945-321 did not come because the
raw amplitudes changed. The change therefore must have been because the
correlator was instructed to calculate GTP and SDO differently. Can you
work out what the change might have been? If you said "the observer changed
the tvchannels", then you're correct! This is one of the crucial takeaways
from this tutorial, and one which will be expanded upon later: **the calibration
of the telescope includes the tvchannel setting, and if you change this
during your observation, the calibration will no longer be the same**. This is
why the correlator remembers the tvchannel range associated with each
different frequency configuration.

## Setting the noise diode amplitude

The last thing we need to do is determine the noise diode amplitude accurately.
To do this we go back to the equation which transforms the correlated output into
the amplitude.

![A_{ij}=\frac{|C_{ij}|\sqrt{N_{i}\times N_{j}}}{\sqrt{SDO_{i}\times SDO_{j}}}](https://latex.codecogs.com/svg.latex?A_%7Bij%7D%3D%5Cfrac%7B%7CC_%7Bij%7D%7C%5Csqrt%7BN_%7Bi%7D%5Ctimes%20N_%7Bj%7D%7D%7D%7B%5Csqrt%7BSDO_%7Bi%7D%5Ctimes%20SDO_%7Bj%7D%7D%7D)

If we have three antennas, we can isolate each antenna's noise diode amplitude
and SDO, and thus allow us to determine it. For example, if we call our antennas
1, 2 and 3, we can get the equations:

![\frac{A_{12}A_{13}}{A_{23}}=\frac{N_{1}}{SDO_{1}}\frac{|C_{12}| |C_{13}|}{|C_{23}|}](https://latex.codecogs.com/svg.latex?%5Cfrac%7BA_%7B12%7DA_%7B13%7D%7D%7BA_%7B23%7D%7D%3D%5Cfrac%7BN_%7B1%7D%7D%7BSDO_%7B1%7D%7D%5Cfrac%7B%7CC_%7B12%7D%7C%20%7CC_%7B13%7D%7C%7D%7B%7CC_%7B23%7D%7C%7D)

![\frac{A_{12}A_{23}}{A_{13}}=\frac{N_{2}}{SDO_{2}}\frac{|C_{12}| |C_{23}|}{|C_{13}|}](https://latex.codecogs.com/svg.latex?%5Cfrac%7BA_%7B12%7DA_%7B23%7D%7D%7BA_%7B13%7D%7D%3D%5Cfrac%7BN_%7B2%7D%7D%7BSDO_%7B2%7D%7D%5Cfrac%7B%7CC_%7B12%7D%7C%20%7CC_%7B23%7D%7C%7D%7B%7CC_%7B13%7D%7C%7D)

![\frac{A_{13}A_{23}}{A_{12}}=\frac{N_{3}}{SDO_{3}}\frac{|C_{13}| |C_{23}|}{|C_{12}|}](https://latex.codecogs.com/svg.latex?%5Cfrac%7BA_%7B13%7DA_%7B23%7D%7D%7BA_%7B12%7D%7D%3D%5Cfrac%7BN_%7B3%7D%7D%7BSDO_%7B3%7D%7D%5Cfrac%7B%7CC_%7B13%7D%7C%20%7CC_%7B23%7D%7C%7D%7B%7CC_%7B12%7D%7C%7D)

That is, if you take the product of the two baselines that involve the antenna
you want to isolate, and divide that by the baseline that doesn't, you cancel out
the effects of the other two antennas; this is called a "gain triplet".

If you know the amplitude of the source you're observing, and it's a point source,
then the left-hand-side of these equations simplifies to give just the amplitude
of the source. And if there are more than three antennas, you can isolate a single antenna
in multiple ways (with six antennas, you can find ten different ways) and thus
get a more accurate estimate of the noise diode amplitudes.

This is what the correlator does when you do an acal, so let's run through
that command. The acal command is a CACOR command, and it requires that you supply
the flux density of the source in each of the IFs. Since most observers use 1934-638 or
0823-500 to do their amplitude calibration, they also usually choose to use CAOBS
to perform the acal because it will automatically supply the appropriate flux densities
of these sources, at the centre frequency of each IF. For example, with centre frequencies
of 5500 MHz and 7500 MHz, CAOBS's flux density model for 0823-500 will automatically
supply the flux densities of 2.622 Jy and 1.747 Jy respectively. That is, you could
type `corr acal` in CAOBS to get the same effect as `acal 2.622 1.747` in CACOR.

The acal command in NVIS has the same functionality, so let's try it. Make sure the
time range you've selected with the `data` command is during the 0823-500 observation,
and then give NVIS the command:

```
NVIS> acal all
```

If you watch the terminal that `rpfitsfile_server` is running in, you'll see a bunch
of output, including the lines:

```
 Data obtained, computing parameters...
  DEBUG: assuming source flux density of 2.622 Jy in window 1
  DEBUG: assuming source flux density of 1.747 Jy in window 2
```

From this you can see it is using the same flux density model as CAOBS does for
0823-500. After a bit of computation on the server, you will see some output in
NVIS similar to:

```
NVIS> acal all
 BAND 1 FLUX = 2.622 Jy, MJD 0.000000 - 100000.000000:
   ANT 1: X = 22.28 Y = 22.51 Jy
   ANT 2: X = 18.73 Y = 19.75 Jy
   ANT 3: X = 18.53 Y = 18.22 Jy
   ANT 4: X = 18.76 Y = 18.76 Jy
   ANT 5: X = 17.21 Y = 15.47 Jy
   ANT 6: X = 21.11 Y = 20.56 Jy
 BAND 2 FLUX = 1.747 Jy, MJD 0.000000 - 100000.000000:
   ANT 1: X = 33.90 Y = 33.50 Jy
   ANT 2: X = 26.86 Y = 27.16 Jy
   ANT 3: X = 28.90 Y = 30.81 Jy
   ANT 4: X = 29.28 Y = 29.46 Jy
   ANT 5: X = 22.70 Y = 20.43 Jy
   ANT 6: X = 29.93 Y = 30.49 Jy
```

These numbers are the computed noise diode amplitudes for each polarisation on
each antenna, for each of the IFs. Once these have been computed, NVIS asks
the server to recompute the amplitudes for all the data. After that has been completed,
the NVIS display should look something like the following picture:

![NVIS display after the acal](nvis_t3_after_acal_aOnSG.png)

The amplitudes on all the baselines are now consistent with each other, and match
the flux density of 0823-500. The "Noise Cal." panel also shows that the amplitude
of each noise diode matches the number in the above NVIS output. However, it doesn't
look like any of the other panels have changed, and they haven't. The simulation
software makes available both the correlator-computed GTP, SDO and system temperature
with the `G`, `O` and `S` panels respectively, and the same quantities computed with
whatever settings you want, with the `g`, `o`, and `C` panels respectively. Take some
time and compare the correlator-computed values with the values computed by the
simulator. For example, here is the comparison between the correlator-computed and
simulator-computed system temperatures.

![NVIS display comparing the system temperatures after the acal](nvis_t3_after_acal_aSC.png)

The system temperatures on each antenna are now computed to be very similar to each other,
which is what we expect at these frequencies. Success!

## The problems with acal

As we saw before, tracking system temperature allows us to keep the system linear
while conditions change. System temperature comes in handy during imaging and analysis,
so you can downweight less sensitive data compared to the more sensitive data. For
these purposes, it doesn't really matter if the system temperature is accurate, so long
as all the data is on the same system temperature scale. In the above plot, we can see
that the flux density of 0823-500 is 2.622 Jy, and that of 0945-321 is about 0.28 Jy.
What happens if we tell the acal that the flux density of 0823-500 is actually 5 Jy at 5.5 GHz?

To do this, first reset the amplitude calibration with `reset acal` in NVIS, and then
`acal 5 3.331 all` (which scales IF2 by the same amount as IF1). You might get something
like the following image.

![NVIS display comparing the system temperatures after the acal with higher amplitude](nvis_t3_bigger_acal_aSC.png)

The computed system temperature is higher than it was before, but so is the flux density
of 0945-321; it's now about 0.55 Jy. So when we told the system to increase the flux
density of 0823-500 by a factor of about 1.9, the flux density of 0945-321 also increased
by that same factor.

This is entirely fixable in data reduction, because of the linearity. If during data
reduction, we decide the flux density scale is incorrect, we simply scale everything the
correct way (this is what gpboot does in Miriad). But Miriad at least does this in a very
simple way, by scaling the gains without adjusting the underlying system temperatures. This
will be no problem if we make an image using data from a single epoch, all obtained after
a single acal. There, if we weight inversely proportionally to system temperature, all
the data will be weighted correctly relative to each other.

Imagine though that you're trying to make an image from several epochs of data, spread
over several weeks or months of time. At each epoch another acal is performed, so each
epoch's system temperature scaling is different. Now, it might be that the same system
temperature from two different epochs could actually represent two different actual
sensitivities.

How different could they be? Let's investigate.

We already saw earlier that a system temperature change could be caused by changing the
tvchannels, so let's start with that. In NSPD, we can clearly see that there's some
pretty strong RFI at about 5580 MHz, especially on the shortest baseline 1-2. If during
the calibration process the observer wanted to avoid this RFI, they may have changed
the tvchannels before the acal. The RFI sits at about channel 1100, so maybe they wanted
to keep as many channels as they could, and chose a range of 200 - 1050.

In this case, the acal will produce system temperatures on average 1.9% lower than
before. If we examine how stable the system temperature is though, we can see that
in the last 2 minutes of 0823-500 data, the system temperature doesn't vary by more
than 0.8% (except on CA01, where it varies by 2%). So the settings used for the acal
produce a significant, measurable effect on the system temperature. And if you're
relying on the system temperature during your data reduction, there will be a
systematic error if it isn't calibrated exactly the same way each time. It may not
be an important or significant error, but it is at least something to keep in
mind when executing your observations.

It can be a challenge to ensure the exact same calibration each time as well. If the
flux density of the source you're using to calibrate changes with time, then using
the same flux density parameters with acal at each epoch is not correct. Only 1934-638
has a stable flux density, while all other calibrators vary, albeit some of them
(like 0823-500) slowly and relatively predictably. The C007 project monitors the
ATCA calibrator list and makes available historical flux densities in the
[ATCA calibrator database](https://www.narrabri.atnf.csiro.au/calibrators/). This
same database also makes it possible to query the most recent flux densities of
any calibrator at any particular set of frequencies (most conveniently through the
CABB scheduler interface).

The data we've been looking at was obtained on 2021-April-28, and we can use the
[flux density request function on the 0823-500 page](https://www.narrabri.atnf.csiro.au/calibrators/calibrator_database_viewcal?source=0823-500#fluxRequest)
to find out that 0823-500 had flux densities of 2.835 Jy and 1.993 Jy at 5.5 and
7.5 GHz respectively, as measured on 2021-May-01. This is 8.1% and 14.1% higher
than the model flux densities at those two frequencies respectively. Putting the
measured values in makes the system temperatures 4% and 6.8% higher
respectively.

To put this in perspective, the system temperature at these frequencies at
32 degrees elevation is about 12% higher than at 50 degrees. In effect, weighting
different epochs with inconsistent system temperature scaling would mean
assigning equal weight to observations with quite different elevations.

## Summary

At this point you should understand how calibrating the amplitude can affect
a lot more than just what is displayed in NVIS. Although we got into a lot of
detail about the mathematics used to convert correlation coefficients into
flux densities, the main points to take away from this tutorial are:

1. Apart from the true flux density of the source, the flux densities
produced by the correlator depend on:
   * the numbers used when the acal command was given,
   * the tvchannel settings,
   * the actual sensitivity of each antenna.
2. The system temperature scaling of the flux densities is crucial to
keeping the system linear, which itself is crucial to ensuring that
accurate measurements can be made with the telescope.
3. The primary assumption made with this flux density calibration is that
the noise diode on each antenna has a stable amplitude over the timescale
of a typical observation.
4. Using system temperatures to weight data during data reduction can lead
to undesirable results if using multiple epochs that do not have
consistent system temperature scaling.

With these first three tutorials done you should know how to do the two
most important calibration commands, why we do them, and how to tell if
they've worked. In the next tutorial we'll see if you can put this
knowledge to use and calibrate a difficult dataset. But for now, here's
a few questions and challenges related to the material in this tutorial.

* In the last plot of this tutorial, you can see that the system temperature
of CA02 was quite a lot higher on 0945-321 than the other antennas, despite it
being pretty much the same as the others while on 0823-500. Explain this behaviour.
Hint: this is directly related to another question in the previous tutorial.
(MODERATE)
* If the amplitude is calculated as a function of the correlation coefficient,
the amplitudes of the noise diodes and the SDO levels on the antennas, then why
does the amplitude shown on NVIS depend on whether the delay is calibrated?
(EASY)
* Try to calibrate the amplitudes before delay calibration, and explain what
happens. (MODERATE)
* Do the tutorial again, but switch on tvmedian at the start. Explain the
differences you notice. (MODERATE)



