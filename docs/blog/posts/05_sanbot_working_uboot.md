---
date: 2026-03-14
authors: [Hoog-V]
categories: 
    - Reverse engineering
    - Robots
    - Sanbot
description: >
    Let's try to exploit Sanbot's deepest secrets by running our own U-boot
title: On a Quest to find Sanbot's deepest secrets – Part 4 (Getting U-boot to work)
comments: true
---

In [part 3](04_sanbot_rooting_dirtycow.md) of this quest we discovered the environment variables needed to build U-Boot. Interestingly, they turned out to be **remarkably similar** to the variables used for the Banana Pi M3.

However, our custom-compiled U-Boot still didn't detect the **eMMC memory**, which meant we couldn't yet load any custom OS. But guess what? After digging around for a while I finally found some clues and got it working!

The goal of running **Ubuntu Touch** is getting closer and closer. I can't wait…

## Some backstory

In [part 1](02_sanbot_quest_dump_rom.md) we attempted to run a custom-compiled U-Boot using configuration values we guessed from the **Banana Pi M3**, which is quite similar to this board. Both use the same **SoC, regulators, and memory configuration**, so it seemed like a reasonable starting point.

Unfortunately we got stuck when trying to detect the MMC device:

```

=> mmc list
=> mmc dev 1
Card did not respond to voltage select! : -110
```

After searching around for this error, I eventually stumbled upon [a blog post on the Armbian forum](https://forum.armbian.com/topic/33673-banana-pi-m3-boot-from-emmc/#comment-180624). That thread referenced [a pull request in the Armbian build system](https://github.com/armbian/build/pull/7252).

Following that rabbit hole led me to a patch related to the `sunxi_mmc_can_calibrate` function.

## `sunxi_mmc_can_calibrate`

In newer releases of U-Boot, the function `sunxi_mmc_can_calibrate` was introduced. Its job is to **automatically calibrate MMC controller timings**.

However, the U-Boot team never added an entry for the **Allwinner A83T**, meaning the calibration logic simply **fails by default** on this SoC. As a result, the `mmc dev` command fails because the handshake never completes, the calibration function returns false, preventing the controller from initializing correctly.

The fix turned out to be surprisingly simple: add support for the **A83T** in the `sunxi_mmc_can_calibrate` logic.

With this small patch:

```diff
From 335c35e6f56b87397d5b6ba74d7676e37269636b Mon Sep 17 00:00:00 2001
From: leo <leo@localhost.localdomain>
Date: Sun, 15 Sep 2024 10:50:38 +0300
Subject: [PATCH 1/2] Add MACH_SUN8I_A83T to can calibrate

Add the A83T processor to the sunxi_mmc_can_calibrate
logic function for proper configuration.

---
 drivers/mmc/sunxi_mmc.c | 1 +
 1 file changed, 1 insertion(+)

diff --git a/drivers/mmc/sunxi_mmc.c b/drivers/mmc/sunxi_mmc.c
index 9534f9ac35..9493bd8639 100644
--- a/drivers/mmc/sunxi_mmc.c
+++ b/drivers/mmc/sunxi_mmc.c
@@ -62,6 +62,7 @@ static bool sunxi_mmc_can_calibrate(void)
 	       IS_ENABLED(CONFIG_MACH_SUN50I_H5) ||
 	       IS_ENABLED(CONFIG_SUN50I_GEN_H6) ||
 	       IS_ENABLED(CONFIG_SUNXI_GEN_NCAT2) ||
+	       IS_ENABLED(CONFIG_MACH_SUN8I_A83T) ||
 	       IS_ENABLED(CONFIG_MACH_SUN8I_R40);
 }
 
-- 
2.35.3
```

…the MMC controller finally came to life:

```
=> mmc list
mmc@1c0f000: 0
mmc@1c10000: 2
mmc@1c11000: 1
=> mmc dev 1
switch to partitions #0, OK
mmc1(part 0) is current device
=> mmc info  
Device: mmc@1c11000
Manufacturer ID: 11
OEM: 0
Name: 016G70 
Bus Speed: 52000000
Mode: MMC High Speed (52MHz)
Rd Block Len: 512
MMC version 5.0
High Capacity: Yes
Capacity: 14.7 GiB
Bus Width: 8-bit
Erase Group Size: 512 KiB
HC WP Group Size: 4 MiB
User Capacity: 14.7 GiB WRREL
Boot Capacity: 4 MiB ENH
RPMB Capacity: 4 MiB ENH
Boot area 0 is not write protected
Boot area 1 is not write protected
=> mmc part

Partition Map for mmc device 1  --   Partition Type: DOS

Part    Start Sector    Num Sectors     UUID            Type
  1     5382144         25493504        00000000-01     0b Boot
  2     73728           65536           00000000-02     06
  3     1               5242880         00000000-03     05 Extd
  5     139264          32768           00000000-05     83
  6     172032          32768           00000000-06     83
  7     204800          4194304         00000000-07     83
  8     4399104         65536           00000000-08     83
  9     4464640         32768           00000000-09     83
 10     4497408         65536           00000000-0a     83
 11     4562944         524288          00000000-0b     83
 12     5087232         32768           00000000-0c     83
 13     5120000         32768           00000000-0d     83
 14     5152768         1024            00000000-0e     83
 15     5153792         31744           00000000-0f     83
 16     5185536         163840          00000000-10     83
 17     5349376         32768           00000000-11     83
=> 
```

Finally! With working MMC access we can now explore the **Sanbot's internal storage directly from U-Boot**, opening the door for booting our own operating system.

The source code for this modified U-Boot can be found on GitHub:
[opendutchsolutions/u-boot-sanbot](https://github.com/opendutchsolutions/u-boot-sanbot)
