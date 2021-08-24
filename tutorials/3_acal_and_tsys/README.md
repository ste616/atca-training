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

