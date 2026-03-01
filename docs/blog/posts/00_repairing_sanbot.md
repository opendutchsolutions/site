---
date: 2026-02-18
authors: [Hoog-V]
categories: 
    - Repair work (Robots)
    - Robots
    - Sanbot
description: >
    Repairing a Sanbot Elf that won’t charge due to a defect in the power supply of this Chinese robot.
title: Repairing a Sanbot Elf Robot That Doesn’t Charge Anymore
comments: true
---

The Sanbot Elf is a Chinese humanoid robot from the company Qihan, launched in 2017. It features a 3D camera, HD camera, an Android tablet (which runs painfully slow due to a mediocre 2018 SoC — the Allwinner A83), animated eyes, a speech engine, LEDs, and motors throughout the body.

![Picture of the Sanbot Elf, a Chinese robot launched in 2017 for around €10,000](assets/sanbot_elf.png)

As some of you might know, I’ve always had an interest in repairing old and obscure hardware. One of my contacts approached me and mentioned he wanted to get rid of the robot because it no longer charged. Of course, I told him I’d be very interested in taking a look at it.

## Prior Work

Before getting my hands on the robot, I searched the internet for information about this hardware. It didn’t take long before I stumbled upon this great [reverse engineering project by Vidicon and Matthijsfh](https://github.com/Vidicon/sanbot_elf_hacking). The repository contains a reverse-engineered block diagram and newly written firmware for the main board and head board.

![Block diagram from the sanbot_elf_hacking repository](assets/sanbot_elf_re_manboard_conn.png)

This gave me the first clue that the Sanbot likely contains a separate power board supplying the system with 12V. Since the robot is mobile and powered by a 12V system, it likely uses a 4S Li-ion battery pack. From the sanbot_elf_hacking repository, it becomes clear that the battery communicates its status over SMBus. That brought back memories of medical battery packs that use a similar system, often with a rather annoying undervoltage lockout mechanism. Once triggered, that lockout can permanently disable the battery. Luckily for us the IC used in the BMS is a BQ3055. Which is this 2-4-series Li-ion battery pack manager containing a fuel-guage. Which as far as I could see did not have this annoying UVLO protection.

While digging a bit deeper, I also found [this useful block diagram from Igor Lirussi’s thesis](https://amslaurea.unibo.it/id/eprint/19120/1/lirussi_igor_tesi.pdf). Which provides a more complete overview of the entire system.

![Full system block diagram](assets/sanbot_elf_full_block_diagram.png)

This block-diagram gives away that it does everything over USB with easy to debug protocols. It gave me hope that we might eventually be able to upcycle the robot by replacing the tablet with our own tablet or computer, running custom software that communicates with the rest of the system.

## Disassembling

From the battery compartment in the back, which is secured with one screw. You can unscrew the full back-cover. Afterwards you can try to wiggle/pull it off. It has locking tabs, so it might feel a bit uncomfortable trying to get it off.

When it's off you will find two circuit boards at the back. The top one is the Mainboard and the other one is the power board.

## Troubleshooting the battery


## Troubleshooting the power board

