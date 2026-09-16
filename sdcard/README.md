# SD Card Assets

Big Dirty loads UI graphics from the Teensy 4.1 built-in SD card.

The five **warning-icon runtime assets have now been recovered exactly** and are preserved in [warning-icons-rgb565.zip](warning-icons-rgb565.zip). The only known missing runtime graphic is the original 480x320 boot logo, `/bowtie.raw`.

## Required files

Current source expects these filenames in the **SD-card root**:

| File | Dimensions | Status | Use |
|---|---:|---|---|
| `bowtie.raw` | 480x320 | **not yet recovered** | boot logo |
| `brake.raw` | 96x72 | recovered | brake warning |
| `checkengine.raw` | 96x72 | recovered | check-engine warning |
| `highbeam.raw` | 96x72 | recovered | high-beam indicator |
| `4WD.raw` | 96x72 | recovered | 4WD indicator |
| `seatbelt.raw` | 96x72 | recovered | seat-belt warning |

To rebuild the SD card, extract the five `.raw` files from `warning-icons-rgb565.zip` directly into the SD-card root.

## Warning-icon format — confirmed

The recovered metadata states:

- 96x72
- RGB565
- 16 bits per pixel
- 13,824 bytes per file
- **little-endian**

This exactly matches the byte order used by `Overlay_Warnings.cpp`:

    uint8_t lo = first byte;
    uint8_t hi = second byte;
    uint16_t color = (hi << 8) | lo;

See [WARNING_ASSET_MANIFEST.md](WARNING_ASSET_MANIFEST.md) for SHA-256 hashes.

## Warning layout

Current `Overlay_Warnings.cpp` places the five 96-pixel-wide slots at:

- X 0
- X 96
- X 192
- X 288
- X 384

with Y = 242.

Current mapping:

- X 0: `brake.raw`
- X 96: `checkengine.raw`
- X 192: `highbeam.raw`
- X 288: `4WD.raw`
- X 384: `seatbelt.raw`

## Boot-logo byte order still unresolved

The warning path is now confirmed little-endian.

The fullscreen `RawImage.cpp` path still names its first byte `hi` and second byte `lo`, while comments describe the source image as little-endian. The original `bowtie.raw` is still missing, so **do not change the boot loader yet**. The known-good boot image should be recovered first and treated as the authority.
