# Tutorial 4
Can you calibrate these challenging data sets? If so, you'll be fine when
you come to do real observing.

# Goals of the tutorial

Put what you have learned in the first three tutorials to use, to tackle some
more challenging initial calibrations.

## Starting the tools for this tutorial

It is assumed that you have already completed both [tutorial 1](../1_introduction/),
[tutorial 2](../2_dcal_and_delavg/), and [tutorial 3](../3_acal_and_tsys),
and so have already obtained and compiled the tools.

Once again, we open up three terminals. In one of these terminals, navigate to
the path `atca-training/tutorials/4_challenging_calibration` and in the other two
go to `atca-training/build`.

You should download the RPFITS file that we will use for this tutorial from
our website:
[2021-10-09_0514.C999](https://www.narrabri.atnf.csiro.au/people/Jamie.Stevens/atca-training-tutorials/4_challenging_calibration/2021-10-09_0514.C999).

Put it into the `atca-training/tutorials/4_challenging_calibration` directory.

## The challenges

### Challenge 1

From that same directory, start the `rpfitsfile_server` with the command:
```bash
../../build/rpfitsfile_server -n 2021-10-09_0514.C999 -j 59496.22465278 -J 59496.22581019
```

The `-j` and `-J` arguments tell the server to make only a subset of the data
in the file available, since there is more data before and after the calibration
that was done online. However we don't need to worry about what the observer did,
we want to calibrate the data ourselves.

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

![NSPD upon startup](nspd_t4_startup.png)

![NVIS upon startup](nvis_t4_startup.png)

#### Goal

In this dataset, we are observing 1934-638 with the 16cm receiver, for just a couple
of minutes. You should be able to use the tools to work out everything else about the
observation and the data.

Doing the initial delay, amplitude and phase calibrations can be tricky at 16cm, since
RFI can get in the way. Using any time range you think best for NVIS calibration, and any reference
antenna, get the data to look very similar to the following images.

![NVIS AA at successful completion of goal 1](nvis_t4_goal1_aa.png)

![NVIS CC at successful completion of goal 1](nvis_t4_goal1_cc.png)

![NSPD IF1 at successful completion of goal 1](nspd_t4_goal1_if1.png)

![NSPD IF2 at successful completion of goal 1](nspd_t4_goal1_if2.png)

Specifically, you should ensure that:
* phases (except for RFI) should remain between +/- 100 degrees across the entire
band
* the system temperatures, as computed by the server, should all be between 18 and 20 K
for each antenna, polarisation and IF

### Challenge 2

For the second challenge, we will revisit the same dataset that we looked at for
the second and third tutorials. Quit the `rpfitsfile_server` you started above and
restart it (from the same directory) with the command:
```bash
../../build/rpfitsfile_server -n ../2_dcal_and_delavg/2021-04-28_0445.C999
```

When you see the `Waiting for connections...` output, you can restart NSPD and NVIS
as you did above.

#### Goal

In our previous tutorials we examined how calibration worked primarily with 0823-500,
which has sufficient flux density to make calibration pretty easy. However, if you
were stuck with a source with low flux density, like 0945-321, could you do as good
a job with the calibration?

It is possible to get the calibration to look like the following images, and you must
achieve this as well, using only data before 04:48 for calibration.

![NVIS AA at successful completion of goal 2](nvis_t4_goal2_aa.png)

![NVIS BB at successful completion of goal 2](nvis_t4_goal2_bb.png)

![NVIS CC at successful completion of goal 2](nvis_t4_goal2_cc.png)

![NVIS DD at successful completion of goal 2](nvis_t4_goal2_dd.png)

![NSPD IF1 at successful completion of goal 2](nspd_t4_goal2_if1.png)

![NSPD IF2 at successful completion of goal 2](nspd_t4_goal2_if2.png)

Specifically, you should ensure that:
* phases, except for RFI and noise, should remain between +/- 100 degrees across
the entire band, for both 0945-321 and 0823-500
* the system temperatures, as computed by the server, should all be between 17 and
26 K for each antenna, polarisation and IF
* the amplitudes displayed in NVIS for 0823-500 should be consistent with the
knowledge that its actual flux density in this epoch was 2.835 Jy at 5.5 GHz and
1.993 Jy at 7.5 GHz

## Some advice

This tutorial is not meant to be tricky, and the data has no major or insurmountable
issues. Everything can be calibrated using
judicious use of tvchannel, refant, delavg, tvmedian and an appropriate time
selection.

What you should get from this tutorial is a better understanding of how to
evaluate your progress while doing the calibration. It is OK to use multiple
dcal commands, so long as at each point the situation improves, and between
each dcal you adjust your parameters (like tvchannel and delavg) to allow you
to progress to an even better calibration. You should also then be aware of when a
calibration is not required.

Look carefully at NSPD and NVIS, and what they are showing you about the data.
Take your time, and try the challenges more than once, using different parameters
each time.

The real goal of this tutorial is to ensure that you develop your data quality
recognition skills, so that when you come to observe for real, you can confidently
calibrate the telescope.

