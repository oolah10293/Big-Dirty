# SD Card Assets

Big Dirty loads UI graphics from the Teensy 4.1 built-in SD card.

The exact known-good binary assets were not recovered into this repository during the September 2026 documentation pass. Do not generate arbitrary replacements and assume they prove the byte order or color path.

## Required files

Current source expects these filenames in the SD-card root:

| File | Dimensions | Use |
|---|---:|---|
| bowtie.raw | 480x320 | boot logo |
| brake.raw | 96x72 | brake warning |
| checkengine.raw | 96x72 | check-engine warning |
| highbeam.raw | 96x72 | high-beam indicator |
| 4WD.raw | 96x72 | 4WD indicator |
| seatbelt.raw | 96x72 | seat-belt warning |

## Pixel format

The project uses 16-bit RGB565 with no image header.

Warning-icon loader behavior is explicitly low byte first, high byte second.

The fullscreen RawImage.cpp path currently reads the bytes in the opposite named order even though comments describe little-endian data. Because the working boot asset is missing from Git, do not change the loaders until an original working RAW file is available.

## Warning layout

Current source places slots at X 0, 96, 192, 288, 384 and Y 242.

Mapping:

- 0: brake.raw
- 96: checkengine.raw
- 192: highbeam.raw
- 288: 4WD.raw
- 384: seatbelt.raw

## When originals are recovered

Commit them under this sdcard/ directory for preservation while keeping runtime copies in the SD-card root.

Useful metadata to record:

- byte size
- SHA-256
- known display screenshot
- confirmed byte order
