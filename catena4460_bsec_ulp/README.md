# catena4460_bsec_ulp

MCCI LoRaWAN test program for the BME680, based on the Bosch Sensortec bsec library `bsec_config_state_ulp_plus.ino`example.

## What we changed

We save the calibration data in FRAM using MCCI's reliable storage library; and we use the Catena-Arduino-Platform library. We send data up to the host every six minutes; we only forward the most recent readings captured when `iaqSensor.run()` returned true (every 5 minutes or so). We use the CPU's deep sleep to delay between 3-second samples. We switched to using `gCatena.SafePrintf()` because that has the logic to deal with USB suspend, unplug, etc.

## Setup

Kind of complicated.

We have tried this procedure with Arduino IDE 1.8.5, and we were successful. However, this did not work with Visual Micro.

- [ ] get the Bosch BSEC distribution (accepting the license) from [this page](https://www.bosch-sensortec.com/bst/products/all_products/bsec#). Scroll to the bottom to accept the license, and you'll get a download prompt.
- [ ] Unzip to a work directory (outside the Arduino working area).
- [ ] Unzip the nested `Arduino/bsec.zip` file into your user Arduino `libraries` directory as `libraries/bsec`.
- [ ] Copy `algo/bin/Normal_version/gcc/Cortex_M0+/libalgobsec.a` to `libraries/bsec/src/cortex-m0plus`.
- [ ] Copy `config/generic_33v_3s_4d/*.{c,h}` to `libraries/bsec/src`.  (If you prefer, you may copy these to the sketch directory, but generally we use a single configuration that's stored in the BSEC library directory.)
- [ ] Edit `platform.txt` for your BSP. For SAMD21 in the MCCI BSP, change the `recipe.c.combine.pattern`:

   ```ini
   recipe.c.combine.pattern="{compiler.path}{compiler.c.elf.cmd}"  "-L{build.path}" {compiler.c.elf.flags} "-T{build.variant.path}/{build.ldscript}" "-Wl,-Map,{build.path}/{build.project_name}.map" --specs=nano.specs --specs=nosys.specs {compiler.ldflags} -o "{build.path}/{build.project_name}.elf" {object_files} -Wl,--start-group {compiler.arm.cmsis.ldflags} "-L{build.variant.path}" -lm {compiler.c.elf.extra_flags} "{build.path}/{archive_file}" -Wl,--end-group
   ```

- [ ] Download a patched arduino-builder, [arduino-builder-219.zip](http://downloads.arduino.cc/PR/arduino-builder/arduino-builder-219.zip). Use the version of `arduino-builder` that's appropriate for your system to replace the file that's already in your Arduino program directory (Windows `c:\Program Files (x86)\Arduino`). Rename the old file first, for safety.

- [ ] Restart the Arduino environment.
- [ ] Build.

## Bugs

The Bosch documentation doesn't make it clear when to use a different calibration file with longer period. The Catena 4460 runs at 3.3V, so we definitely need the 3.3v family. We've only tested with the 3s / 4d combination.

Adafruit and MCCI use a different default address for the BME680 than Bosch does.
