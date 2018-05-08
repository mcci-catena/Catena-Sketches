# catena4460_bsec_ulp

MCCI port of the Bosch Sensortec bsec library `bsec_config_state_ulp_plus.ino`example.

## Setup

Kind of ugly.

You can clone https://gitlab-x.mcci.com/client/milkweed/mcgraw/bsec.git if you have access. Outside MCCI, you'll have to do the following. (Note that the following insructions are for SAMD21 things like the Feather M0.)

- [ ] get the Bosch BSEC distribution (accepting the license)
- [ ] Unzip
- [ ] Unzip the nested `Arduino/bsec.zip` file into your Arduino `libraries` directory as `libraries/bsec`.
- [ ] Copy `algo/bin/Normal_version/gcc/Cortex_M0+/libalgobsec.a` to libraries/bsec/src/cortex-m0plus`.
- [ ] Copy `config/gnereic_33v_300s_28d/*.{c,h}` to libraries/bsec/src.
- [ ] Edit `platform.txt` for your BSP. For SAMD21 in the MCCI BSP, change the `recipe.c.combine.pattern`:

   ```ini
   recipe.c.combine.pattern="{compiler.path}{compiler.c.elf.cmd}"  "-L{build.path}" {compiler.c.elf.flags} "-T{build.variant.path}/{build.ldscript}" "-Wl,-Map,{build.path}/{build.project_name}.map" --specs=nano.specs --specs=nosys.specs {compiler.ldflags} -o "{build.path}/{build.project_name}.elf" {object_files} -Wl,--start-group {compiler.arm.cmsis.ldflags} "-L{build.variant.path}" -lm {compiler.c.elf.extra_flags} "{build.path}/{archive_file}" -Wl,--end-group

   ```

- [ ] Download a patched arduino-builder, [arduino-builder-219.zip](http://downloads.arduino.cc/PR/arduino-builder/arduino-builder-219.zip). Use the version of `arduino-builder` that's appropriate for your system to replace the file that's already in your Arduino program directory (Windows `c:\Program Files (x86)\Arduino`). Rename the old file first, for safety.

- [ ] Restart the Arduino environment.
- [ ] Try to build.

## Expected output

If it works, you'll see something like the following, and a fast flash frmo the LED:

```console
+Catena4460::begin()
isValid: end pointer set to 0x258
+CatenaSamd21::begin()
+CatenaSamd21::begin(0, 0)
Catena4460::GetPlatformForID entered

BSEC library version 1.4.6.0
Zeroing state
Timestamp [ms], raw temperature [°C], pressure [hPa], raw relative humidity [%], gas [Ohm], IAQ, IAQ accuracy, temperature [°C], relative humidity [%]
6103, 25.49, 98667.88, 32.42, 119765.79, 25.00, 0, 25.49, 32.42
306101, 25.70, 98669.41, 32.05, 178382.66, 25.00, 0, 25.70, 32.05
606101, 25.57, 98663.88, 31.91, 202627.42, 25.00, 0, 25.57, 31.91
906101, 25.70, 98661.56, 31.82, 218794.86, 25.00, 0, 25.70, 31.82
1206101, 25.71, 98660.85, 31.90, 227339.77, 25.00, 1, 25.71, 31.90
1506101, 25.85, 98663.73, 31.66, 233253.03, 25.00, 1, 25.85, 31.66
1806101, 25.85, 98652.70, 31.59, 236748.06, 25.00, 1, 25.85, 31.59
2106101, 25.65, 98653.56, 31.94, 240698.17, 25.00, 1, 25.65, 31.94
2406101, 25.77, 98650.87, 31.76, 242989.67, 25.00, 1, 25.77, 31.76
2706101, 25.76, 98648.59, 31.63, 243882.70, 25.00, 1, 25.76, 31.63
3006101, 25.82, 98652.58, 31.53, 245506.80, 25.00, 1, 25.82, 31.53
3306101, 25.96, 98648.32, 31.38, 245870.64, 25.00, 1, 25.96, 31.38
3606101, 25.78, 98660.78, 31.51, 246052.92, 29.34, 1, 25.78, 31.51
3906101, 25.81, 98660.62, 31.54, 246784.98, 25.00, 1, 25.81, 31.54
4206101, 25.76, 98661.97, 31.57, 246784.98, 26.59, 1, 25.76, 31.57
4506101, 25.62, 98664.92, 31.77, 247336.86, 25.00, 1, 25.62, 31.77
```

## Bugs

The documnentation doesn't make it clear whether we should be using a different calibratin file. 

Adafruit and MCCI use a different default address for the BME680 than Bosch does.
