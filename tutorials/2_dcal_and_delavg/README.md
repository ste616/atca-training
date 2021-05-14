# Tutorial 2
A more detailed look at delay calibration.

## Goals of the tutorial

In this tutorial, you will learn how delay calibration works in more detail,
including the effects of different tvchannel settings, source flux density and
delay averaging.

## Starting the tools for this tutorial

It is assumed that you have already completed [tutorial 1](../1_introduction/) and
so have already obtained and compiled the tools.

Once again, we open up three terminals. In one of these terminals, navigate to
the path `atca-training/tutorials/2_dcal_and_delavg` and in the other two go to
`atca-training/build`.

You should download the RPFITS file that we will use for this tutorial from
our website:
[2021-04-28_0445.C999](https://www.narrabri.atnf.csiro.au/people/Jamie.Stevens/atca-training-tutorials/2_dcal_and_delavg/2021-04-28_0445.C999). Put it into the
`atca-training/tutorials/2_dcal_and_delavg` directory.

From that same directory, start the `rpfitsfile_server` with the command:
```bash
../../build/rpfitsfile_server -n 2021-04-28_0445.C999
```

When you see the `Waiting for connections...` output, you can start using the other
tools.

Start NSPD in one of the other terminals:
```bash
./nspd -d /xs -s 127.0.0.1 -u usr123
```

You should feel free to change `-d /xs` to use a different interactive PGPLOT device
if you'd like, and to change `-u usr123` to use a different username.

Start NVIS in the third terminal:
```bash
./nvis -d /xs -s 127.0.0.1 -u usr123
```

Once again, you can change the PGPLOT device to whatever you want, but you should
make sure if you changed `-u usr123` for NSPD, do the same here for NVIS.

At this point NSPD and NVIS should look something like the following two images.

![NSPD upon startup](nspd_t2_startup.png)

![NVIS upon startup](nvis_t2_startup.png)

## The observation

In this dataset, two sources were observed. From roughly 04:46 to 04:48:15 we
observed the source 0945-321 which has a flux density of about 323 mJy at 5.5 GHz,
and from about 04:49:30 until 04:52:10 we observed 0823-500, which has a flux
density of about 2.879 Jy at 5.5 GHz (a ~9 times higher flux density than 0945-321).

We are also observing with a 5 second cycle time, to really ensure that the
per-cycle signal-to-noise ratio of 0945-321 is low. At 5.5 GHz in 5 seconds, on
a single baseline, in a single 1 MHz channel, the ATCA RMS noise level is around
157 mJy.

We're going to use these two different sources to explore delay calibration
strategies.

## Normal strategy

Let's begin by calibrating using 0823-500, which is a normal thing to do in
the 4cm band when 1934-638 is not available. In NSPD, look at the time 04:51
with `get time 04:51`. You should see NSPD display the timestamp 04:51:02,

Normally, when preparing for delay calibration, we will look at phase. Let's
configure NSPD to show us all of our IF1 phases with `nxy 3 7` and
`sel aa bb ab` and `p -180 180`. You will see that the phases are "wrapping"
at different rates on different baselines and polarisations, but that the
phase itself is quite well defined in each channel to make it look like
a sawtooth pattern. You can zoom in on a smaller channel range if you'd like
to confirm this is the case on the products where the phase is wrapping more
rapidly.

You should see much the same thing if you look at IF2 with `sel f2`, although
the wrapping rates will not necessarily be anything like what you saw in IF2.
This is because both IFs are, after the first conversion stage, completely
independent of each other.

In [tutorial 1](../1_introduction/) we talked about how the change of phase
with frequency is due to a delay error being present when the correlator
was doing its fringe rotation. Let's take a quick look at how that works
mathematically. For any particular time period &Delta;t, that represents
some amount of phase &Delta;&#981; for a wave with some frequency &nu;.
The relationship here is &Delta;&#981; = 2&#960;&nu;&Delta;t. To get the
rate of change of phase with frequency, differentiate; here we dispense with
calling phase &Delta;&#981; since phase can have any zero point that we like.
So &part;&#981; / &part;&nu; = 2&#960;&Delta;t, and rearranging we get
&Delta;t = (1 / 2&#960;) &part;&#981; / &part;&nu;.