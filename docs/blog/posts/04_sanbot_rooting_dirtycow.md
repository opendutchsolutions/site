---
date: 2026-03-09
authors: [Hoog-V]
categories: 
    - Reverse engineering
    - Robots
    - Sanbot
description: >
    Let's try to find Sanbot's deepest secrets by dumping it's rom
title: On a Quest to find Sanbot's deepest secrets – Part 3 (Rooting the tablet and dumping the ROM)
comments: true
---

Part 2 of the quest focused on figuring out how the tablet communicates with its internal control boards. [This blog post will instead elaborate on part 1](02_sanbot_quest_dump_rom.md), where we explored some of the ways the ROM could be dumped. 

Unfornately we did not succesfully dump it in part 1, though now in this post we will be succesfully dumping the ROM using the dirtyCow root method using adb to download it back to pc.

## Very old kernel + old android == exploits

The tablet inside is a custom A83T tablet which uses an allwinner-a83t. It got octacore arm-a7 processors with a Power VR SGX544MP1 gpu, [you can read more about this mediocre ic here](https://linux-sunxi.org/A83T). [Or you can read the rant blog posts on armbian about trying to support this device](https://forum.armbian.com/topic/474-banana-pi-m3/), but maintainers having headaches due to the bad performance and proprietary blobs for the Power VR GPU.

Due to suboptimal hardware choices, the vendor was almost locked down to the 3.14.39 kernel that allwinner (sunxi) provides. On top of the kernel runs a slimmed down version of android 6.0.

Due to the very outdated software, it left quite a range of exploits open for us to try and gain root-access. One of them was dirtyCow. I followed the instructions from [GetRoot-Android-DirtyCow by jOnkO](https://github.com/j0nk0/GetRoot-Android-DirtyCow). This fortunately worked right out of the box.

```bash

$ adb shell run-as
...
...
root@octopus_qh106 # id -u
0
```

With the root shell now working, there was one thing left to do...

## Dump the rom

Dumping the rom with root shell is pretty easy. We need to know what we want to dump, which is easy to find out by running:

```bash
ls /dev/block*
```

From there it is easy to figure out we need to dump mmcblk0. Though dumping the whole mmcblk0 means we get a ~16GB image, which we need to store somewhere. First I thought of using an external USB stick as that might give higher transport speeds then transferring it using WiFi (SDIO-wifi ~ 2Mb/s from what i've tried) or ADB (USB HS 480Mbps). Though quickly I found out, I could only write using the FUSE layer mounted /storage/<USB_NAME> location. 

But by trying to dump it to USB, the FUSE layer gave up and crashed. That's why I eventually settled on transferring using ADB. To dump over adb, I did the following:

1) Set-up reverse forwarding of port 10000
```bash
adb reverse tcp:10000 tcp:10000
```

2) Set-up listening socket on my laptop
```bash
nc -l 10000 > android_dump.img
```

3) Start the root shell using ADB and using dd with pipe to netcat which sends it over the reverse forwarding connection.
```bash
dd if=/dev/block/mmcblk0 | toybox nc -q 0 127.0.0.1 10000
```

After 30 minutes of waiting, I had my firmware dump.