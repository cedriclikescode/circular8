# Circular8
A lightweight player/simulator compatible with Simai (maidata.txt) files.

> [!NOTE]
> Circular8 does not currently have a chart editor. Please use another tool for that purpose instead.

## Features:
- **All** notation/opcodes listed at [Notations of simai](https://w.atwiki.jp/simai/pages/1003.html) plus a few non-standard extensions. Notable ones include:
    - Fake EACH (`3/0`)
        - Note: only supports 0, using 9 will throw an error.
    - Individual slide tracing lengths  (`1-4[4:1]-7[4:3]`)
    - Connected fan slides (`1w5w1[1:1]`)
    - &alias_# (Custom difficulty names)
    - &color_# (Custom difficulty colors)
        - Uses case-insensitive hex code (abbreviated variant also supported)
- **Native** support for Linux® based operating systems
- Supports multiple control methods:
    - ADX Controller (NDX should also work too but untested)
        - Only keyboard mode is currently supported.
        - On Linux®, modification of `drivers/usb/class/cdc-acm.c` is required for the touchscreen to be recognized.
    - Touchscreen
        - Note: if two fingers are on the same sensor A, a button input will also be sent.
        - It is recommended to enable showing sensors to make it easier to press correctly.
- Rating system
    - Currently trusts levels (decimal and + notation supported) set in maidata.txt as fact.
    - Currently utilizes the sha256 of the **entire** maidata.txt
    - Uses custom formula where 97% is mastery of the chart and is interpolated between the current and the next rating table to prevent a x.9999% situation.

## Libraries/thirdparty things used:
- [FFmpeg](https://www.ffmpeg.org/) (LGPL-2.1-or-later)
    - Used: avcodec, avformat, and avutil.
    - Toggleable via the `Circular8_ENABLE_FFmpeg` define option.
    - Also may have parts under GPL-2.0-or-later.
- [inih](https://github.com/benhoyt/inih) (BSD-3-Clause)
- [Noto Sans Japanese](https://fonts.google.com/noto/specimen/Noto+Sans+JP) (OFL-1.1)
- [serial](https://github.com/tisyang/serial) (BSD-3-Clause)
    - Toggleable via the `Circular8_ENABLE_CONTROLLER` define option.
- [SDL3](https://github.com/libsdl-org/SDL) (Zlib)
- [SDL3_image](https://github.com/libsdl-org/SDL_image) (Zlib)
- [SDL3_mixer](https://github.com/libsdl-org/SDL) (Zlib)
- [SDL3_ttf](https://github.com/libsdl-org/SDL) (Zlib)
- [SHA-256](https://github.com/983/SHA-256) (Unlicense)
- [stb](https://github.com/nothings/stb) (Unlicense/MIT)
    - Used: image_resize2 and rect_pack.

## Licenses
- Circular8 source code (files in `src` other than files in `src/thirdparty`): MIT
- Circular8 graphics: CC0
	- Some graphics are obtained from and/or modified from their original sources. See gfx/credits.txt for more details.
- Circular8 sounds: CC0
	- Obtained and/or modified from their original sources. See sfx/credits.txt for details.
- Libraries/thirdparty: see above
	

## Todo list:
- [ ] Use levels provided by a difficulty calculator instead (A **big** todo).
- [ ] sha256 only the selected chart minified and standardized (e.g. removing comments, reordering hold/break/ex combination to be in one specific order, etc).
- [ ] Utilize and modify testffmpeg.c from the SDL sources to have much better video decoding.
- [ ] Apply accurate slide arrow progress displaying.
- [ ] Make slide judgement occur later instead of instantly
- [ ] Add support for <HS*#> and <VS*#>


## Disclaimers
- Some AI assistance was used for a small portion of code.
- Circular8 is **NOT** associated with any companies in any way.