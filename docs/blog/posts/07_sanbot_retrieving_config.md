---
date: 2026-03-16
authors: [Hoog-V]
categories: 
    - Reverse engineering
    - Robots
    - Sanbot
description: >
    Let's try to exploit Sanbot's deepest secrets by dumping the build configuration (script.bin / allwinner fex)
title: On a Quest to find Sanbot's deepest secrets – Part 6 (Dumping the build configuration of the android rom)
comments: true
---

[As we now have our own custom buildroot port running on the Sanbot tablet](06_sanbot_porting_buildroot.md), we ofcourse need the internal screen to be working. 

Which was a long tedious journay with a unexpected twist. Like always things are sometimes easier then I would have thought.

!!! warning annotate "Legal note"

    This research was conducted on hardware legally owned by the author.
    All analysis is performed for the purposes of interoperability,
    repair, and educational research.

    No proprietary firmware or copyrighted software
    is redistributed on this site.

## What do I need?

To get the LCD working under mainline Linux we need one very specific piece of information:

**The original hardware configuration used by the vendor kernel.**

On modern ARM systems this information lives in a **device tree** (`.dtb`).  
But as we discovered earlier, the Sanbot tablet is powered by an **Allwinner A83T**, and the vendor kernel is an ancient **3.4 BSP kernel**.

Those kernels don’t use device trees.

Instead they rely on a configuration format called **FEX**.

The workflow looked roughly like this:

```

sys_config.fex → script.bin → parsed by kernel

```

The `script.bin` contains things like:

- LCD panel configuration
- touchscreen controller
- GPIO mappings
- power regulators
- DRAM configuration
- WiFi chip
- I²C buses

Basically **the entire board layout**.

Since the immediate goal was getting the internal display working, I was mainly interested in the **LCD parameters**.

Those usually live in the `lcd0_para` section of the FEX file and contain values such as:

```

[lcd0_para]
lcd_used = 1
lcd_driver_name = "some_panel"
lcd_if = 0
lcd_x = 720
lcd_y = 1280
lcd_dclk_freq = ?
lcd_pwm_used = ?
lcd_pwm_freq = ?
lcd_hbp = ?
lcd_ht = ?
lcd_vbp = ?
lcd_vt = ?

```

Which translates roughly into the parameters a modern **device tree panel driver** expects:

- resolution (x / y)
- pixel clock
- horizontal timing
- vertical timing
- interface type (RGB / LVDS / MIPI DSI)
- backlight control
- reset GPIO
- power regulators

Without these values the kernel simply has **no idea how to drive the display controller**.

And on tablets, the LCD panel is almost always a **custom OEM part**, which means guessing those numbers is... not fun.

So instead of guessing, the plan was simple:

> Extract the original vendor configuration.

If we could recover the original `sys_config.fex`, we would essentially have **the exact blueprint of the hardware**.

Sounds easy, right?

It wasn't.

## Reading the firmware dump

Earlier in the series we dumped the entire firmware image from the tablet.

So naturally the first idea was:

```

Just search the firmware for script.bin

```

Which unfortunately yielded absolutely nothing.

No `script.bin`.

No `.fex`.

No device tree.

Even `binwalk` came up empty.

At this point I assumed one of two things had happened:

1. The configuration was **compiled directly into the kernel**.
2. The configuration was **embedded in some proprietary binary blob**.

Both of those options are painful.

So I started digging through the kernel strings:

```

strings zImage | grep script

```

And suddenly things started to look interesting.

I found references like:

```

script_get_item
script_parser_fetch
sys_config.fex
ctp_para
lcd0_para
wifi_para

```

Those names are **classic Allwinner FEX sections**.

Meaning the kernel was **definitely expecting a FEX configuration**.

But where was it coming from?

---

## The hint! Fex source code from the BSP

After some digging through the Allwinner BSP sources (which are… let's say *creatively organized*) I stumbled upon something interesting.

The configuration parser used by the kernel is implemented in:

```

script_parser.c

```

Inside the BSP source tree.

And buried in that code is a rather revealing comment:

> The script configuration may be provided by the bootloader or loaded from memory.

Wait.

Loaded from memory?

That means the bootloader can **pass the FEX configuration directly to the kernel at boot time**.

Which means it might not exist anywhere in the filesystem at all.

Instead it might only exist in **RAM during boot**.

That was the moment where things started to click.

---

## It was in RAM all the time... Ugh

After realizing the configuration might only exist in memory, the obvious next step was to inspect what the running kernel exposes.

Linux often exposes hardware configuration through **virtual filesystems** like:

```

/proc
/sys

```

So I started poking around.

And then I noticed something very interesting.

Inside `/proc`:

```

/proc/script/

```

And:

```bash
# ls /proc/script/
dump    get-item
```

Wow, it has something called dump! 

Which means the configuration we were looking for had been available the entire time.

Right there. All we had to do was dump it.

---

## Dumping the configuration

Once we knew where to look, extracting it became surprisingly simple.

On the running system:

```

echo all > /proc/script/dump
cat /proc/script/dump

<System_crash>

```

Uh oh... Why did my system crash... 

Guess what? Buffer overflow...

So let's do it in parts and dump it to sdcard:

```bash
OUT=/sdcard/sys_config_dump.txt
: > "$OUT"

for k in \
product platform target secure charging_type key_detect_en power_sply gpio_bias \
card_boot box_start_os disp_init lcd0_para lcd1_para ctp_para wifi_para bt_para \
camera0_para camera1_para gsensor_para ; do
    echo "$k" > /sys/class/script/dump 2>/dev/null || continue
    {
        echo "===== $k ====="
        cat /sys/class/script/dump
        echo
    } >> "$OUT"
done

sync
echo "saved to $OUT"
```

Great! Now we finally had the mysterious **`script.bin`**.

But this binary format isn't exactly human friendly.

Luckily the sunxi community already solved that problem years ago.

Using the `sunxi-tools` utilities we can convert it back into readable FEX:

```

bin2fex script.bin > sys_config.fex

```

And suddenly the entire board configuration appears in front of you:

```

[lcd0_para]
lcd_used = 1
lcd_driver_name = "qihan_panel"
lcd_x = 720
lcd_y = 1280

[ctp_para]
ctp_used = 1
ctp_name = "gslX680"

[wifi_para]
wifi_used = 1
wifi_sdc_id = 3

```

And just like that we now have the exact information needed to reconstruct the hardware description.

---

## What information was found

The pins:

```
PD2  -> lcdd2
PD3  -> lcdd3
PD4  -> lcdd4
PD5  -> lcdd5
PD6  -> lcdd6
PD7  -> lcdd7
PD10 -> lcdd10
PD11 -> lcdd11
PD12 -> lcdd12
PD13 -> lcdd13
PD14 -> lcdd14
PD15 -> lcdd15
PD18 -> lcdd18
PD19 -> lcdd19
PD20 -> lcdd20
PD21 -> lcdd21
PD22 -> lcdd22
PD23 -> lcdd23

PD24 -> lcdclk
PD25 -> lcdde
PD26 -> lcdhsync
PD27 -> lcdvsync
```

The resolution and pixel-clk:

```
1920x1200
lcd_dclk_freq = 150 MHz
```

The backlight and PWM channel:
```
lcd_bl_en -> PD29
lcd_pwm_used = 1
lcd_pwm_ch = 0
```

The ssd2828 panel driver communication pins:

```
lcd_gpio_0 (SPI_SCK)     -> PE5
lcd_gpio_1 (SPI_MOSI)    -> PE6
lcd_gpio_2 (SPI_CS)      -> PE7
lcd_gpio_4 (RESET)         -> PE9
lcd_gpio_5 (PANEL_POWER)   -> PL10
lcd_gpio_6 (PANEL_EN/IRQ?) -> PH4
```

Brightness mapping:

```
lcd0_backlight = 255
```

LCD driver parameters:

```
lcd_used = 1 
lcd_driver_namestring = "qihan_lcd" 
lcd_if = 4 
lcd_x = 1920 
lcd_y = 1200 
lcd_width = 150 
lcd_height = 94 
lcd_dclk_freq = 150 
lcd_pwm_used = 1 
lcd_pwm_ch = 0 
lcd_pwm_freq = 10000 
lcd_pwm_pol = 1
lcd_hbp = 80 
lcd_ht = 2044 
lcd_hspw = 16 
lcd_vbp = 16 
lcd_vt = 1224 
lcd_vspw = 8 
lcd_hv_clk_phaseint = 1

```

## Why this matters

With the recovered `sys_config.fex` we can now:

- identify the **LCD panel**
- identify the **touchscreen controller**
- determine **GPIO assignments**
- determine **power regulators**
- map **I²C devices**
- reconstruct a **proper device tree**

Which means the next step becomes possible:

> **Getting the internal display working under mainline Linux.**

And that journey turned out to be even more entertaining than this one.

Because of course it did.

---

*To be continued…*

