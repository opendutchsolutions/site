---
date: 2026-03-09
authors: [Hoog-V]
categories: 
    - Reverse engineering
    - Robots
    - Sanbot
description: >
    Let's try to find Sanbot's deepest secrets by dumping its rom
title: On a Quest to find Sanbot's deepest secrets – Part 3 (Rooting the tablet and dumping the ROM)
comments: true
---

Part 2 of the quest focused on figuring out how the tablet communicates with its internal control boards.  
[In this post we return to part 1](02_sanbot_quest_dump_rom.md), where we explored several ways to dump the tablet's ROM.

Unfortunately we were not able to successfully dump it in part 1.  
In this post we finally manage to extract the ROM using the DirtyCow root exploit and `adb`.

!!! warning annotate "Legal note"

    This research was conducted on hardware legally owned by the author.
    All analysis is performed for the purposes of interoperability,
    repair, and educational research.

    No proprietary firmware or copyrighted software
    is redistributed on this site.


## Very old kernel + old android == exploits

The tablet inside the robot is based on an Allwinner A83T SoC.It contains an octa-core ARM Cortex-A7 CPU together with a PowerVR SGX544MP1 GPU [you can read more about this mediocre ic here](https://linux-sunxi.org/A83T). [Or you can read the rant blog posts on armbian about trying to support this device](https://forum.armbian.com/topic/474-banana-pi-m3/), with maintainers having headaches due to the bad performance and proprietary blobs for the Power VR GPU.

Because of the vendor BSP provided by Allwinner, the system is effectively locked to the very old 3.14.39 kernel. On top of this kernel runs a slimmed-down Android 6.0 system.

Because the software stack is extremely outdated, several well-known privilege-escalation exploits remain available. One of them was dirtyCow. I followed the instructions from [GetRoot-Android-DirtyCow by jOnkO](https://github.com/j0nk0/GetRoot-Android-DirtyCow). Which fortunately worked right out of the box.

```bash

$ adb shell run-as
...
...
root@octopus_qh106 # id -u
0
```

With the root shell now working, there was one thing left to do...

## Dump the rom

Dumping the ROM once root access is available is fairly straightforward. The first step is determining which block device contains the eMMC:

```bash
ls /dev/block*
```

From there it is easy to figure out we need to dump mmcblk0. Though dumping the whole mmcblk0 means we get a ~16GB image, which we need to store somewhere. First I thought of using an external USB stick as that might give higher transport speeds then transferring it using WiFi (SDIO-wifi ~ 2Mb/s from what i've tried) or ADB (USB HS 480Mbps). Though quickly I found out, I could only write using the FUSE layer mounted /storage/<USB_NAME> location. 

But by trying to dump it to USB stick, the FUSE layer gave up and crashed. That's why I eventually settled on transferring over ADB. To dump over adb, I did the following:

1) Set-up reverse forwarding of port 10000
```bash
adb reverse tcp:10000 tcp:10000
```

2) Set-up listening socket on my laptop, which dumps data to an android_dump.img file.
```bash
nc -l 10000 > android_dump.img
```

3) Start the root shell using ADB and using dd with pipe to netcat which sends it over the reverse forwarding connection.
```bash
dd if=/dev/block/mmcblk0 | toybox nc -q 0 127.0.0.1 10000
```

After 30 minutes of waiting, I had my firmware dump.

## What's in the dump

Well, all the apps of course, but those were already extracted earlier.. What I was really after were the DRAM, eMMC and power-management configuration settings required to build my own U-Boot.

It was tough finding this information in the image as they compiled the configuration options into u-boot as a static binary. which is notoriously difficult to reverse-engineer.. Though I found these settings;

```yaml
platform:
  soc_family: sunxi
  soc_arch: sun8i
  probable_soc: sun8i-a83t
  bootloader: u-boot
  vendor: allwinner
  secure_mode_strings:
    - SUNXI_SECURE_MODE
    - SUNXI_NORMAL_MODE

boot_environment:
  console: "ttyS0,115200"
  nand_root: "/dev/system"
  mmc_root: "/dev/mmcblk0p7"
  init: "/init"
  loglevel: "4"
  selinux: "disabled"

boot_variables:
  bootdelay: 0
  bootcmd: "run setargs_nand boot_normal"
  boot_normal: "sunxi_flash read 40007800 boot;boota 40007800 boot"
  boot_recovery: "sunxi_flash read 40007800 recovery;boota 40007800 recovery"
  boot_fastboot: "fastboot"

boot_arguments:
  setargs_nand: >
    setenv bootargs
    console=${console}
    root=${nand_root}
    init=${init}
    vmalloc=384M
    ion_cma_list="120m,176m,512m"
    loglevel=${loglevel}
    partitions=${partitions}
    androidboot.selinux=${selinux}

  setargs_mmc: >
    setenv bootargs
    console=${console}
    root=${mmc_root}
    init=${init}
    vmalloc=384M
    ion_cma_list="120m,176m,512m"
    loglevel=${loglevel}
    partitions=${partitions}
    androidboot.selinux=${selinux}

key_triggers:
  recovery_key_value_min: "0x10"
  recovery_key_value_max: "0x13"
  fastboot_key_value_min: "0x2"
  fastboot_key_value_max: "0x8"

mmc_configuration_strings:
  mmc_devices:
    - MMC0
    - MMC2
  mmc_root: "/dev/mmcblk0p7"
  mmc_slots_detected:
    - mmc0
    - mmc1
    - mmc2

  mmc_commands_available:
    - mmc dev
    - mmc list
    - mmc part
    - mmc rescan
    - mmcinfo
    - mmc read
    - mmc write
    - mmc erase

sunxi_flash_commands:
  - sunxi_flash read
  - sunxi flash log_read
  - sunxi flash phy_read

fastboot_support:
  enabled_strings:
    - sunxi fastboot
    - fastboot download
    - fastboot erase
    - fastboot flash

usb_boot_modes:
  efex_mode_strings:
    - SUNXI_USB_EFEX_BOOT0_TAG
    - SUNXI_USB_EFEX_BOOT1_TAG
    - SUNXI_USB_EFEX_MBR_TAG
    - SUNXI_USB_EFEX_ERASE_TAG

  usb_modes:
    - fastboot
    - efex
    - pburn

security_features:
  signature_verification:
    - sunxi_verify_signature
    - sunxi_verify_rotpk_hash
  rsa_engine:
    - sunxi_rsa_calc
  sha_engine:
    - sunxi_sha_calc

secure_storage:
  functions:
    - sunxi_secure_storage_read
    - sunxi_secure_storage_write
    - sunxi_secure_storage_erase
    - sunxi_secure_storage_erase_all
    - sunxi_secure_storage_list

dram_configuration_strings:
  dram_parameters:
    - dram_clk
    - dram_type
    - dram_zq
    - dram_odt_en
    - dram_para1
    - dram_para2
    - dram_mr0
    - dram_mr1
    - dram_mr2
    - dram_mr3
    - dram_tpr0
    - dram_tpr1
    - dram_tpr2
    - dram_tpr3
    - dram_tpr4
    - dram_tpr5
    - dram_tpr6
    - dram_tpr7
    - dram_tpr8
    - dram_tpr9
    - dram_tpr10
    - dram_tpr11
    - dram_tpr12
    - dram_tpr13

pmic_strings:
  pmic: axp81x
  regulators:
    - dcdc1
    - dcdc2
    - dcdc3
    - dcdc4
    - dcdc5
    - fldo1
    - fldo2
    - aldo1
    - aldo2
    - aldo3
    - eldo1
    - eldo2
    - eldo3

boot_modes:
  supported_modes:
    - normal
    - recovery
    - fastboot
    - sprite_test
    - usb_efex

graphics_boot:
  bmp_functions:
    - sunxi_bmp_logo_display
    - sunxi_bmp_charger_display
    - sunxi_bmp_show
    - sunxi_bmp_display

filesystem_boot:
  boot_partition_load_address: "0x40007800"
  boot_image_name: "boot"
  recovery_image_name: "recovery"

misc_strings:
  uboot_prompt: "sunxi#"
  console_uart: "sunxi_serial"
  hardware_string: "sunxi_hardware"
```

And yes they did set the `bootdelay=0` param, so you can't activate any u-boot shell.

The legacy Allwinner BSP used by this Android system relies on **FEX files** to describe the board configuration for both U-Boot and the Android boot image. This predates the now standard device-tree based configuration used in modern kernels.