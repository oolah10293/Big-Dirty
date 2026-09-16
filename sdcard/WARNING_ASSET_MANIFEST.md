# Warning Asset Manifest

This directory now preserves the exact warning-icon bundle recovered from the project on 2026-09-16.

## Canonical bundle

- File: `warning-icons-rgb565.zip`
- Original uploaded archive: `rerawimages.zip`
- Archive size: **10,353 bytes**
- Archive SHA-256: **57de48aacb64067b10c3c130dd77caf760103188a1f07014d84b8083cde1f56c**

The ZIP contains the five runtime warning files plus one small metadata text file for each asset.

## Runtime files

Every warning RAW file is exactly:

- Width: **96**
- Height: **72**
- Format: **RGB565**
- Bits per pixel: **16**
- Endianness: **Little**
- Byte size: **13,824**

| Runtime file | SHA-256 |
|---|---|
| `4WD.raw` | `dacc4b6590ea69b7dc742553665d5e285c9774356831d428dee21bed2711bc68` |
| `brake.raw` | `2974d27131bc4ae19bd96f02ad187742709e9565f89bf955a34b023f238b55e3` |
| `checkengine.raw` | `01a0f97ff17c887ccf78b39f2c8de194ed9f76dd49db9948e9d5150175cc2332` |
| `highbeam.raw` | `5717652b9dcd21f1858558f0a2aa564fd6bbee7000f24b21108501d81bf9314d` |
| `seatbelt.raw` | `0eb9d7384f30dc9aaa6b3e994750c391b8d13d82c959d6940ab9b24ca10a5781` |

## Byte-order confirmation

The metadata included with each recovered asset explicitly identifies the data as **little-endian RGB565**.

That matches `Overlay_Warnings.cpp`, which reconstructs each pixel as:

    lo = first byte
    hi = second byte
    color = (hi << 8) | lo

So the warning-icon byte order is no longer uncertain.

## Still missing

The warning assets are recovered. The remaining missing runtime graphic is:

- `/bowtie.raw` — 480x320 boot logo

The fullscreen `RawImage.cpp` byte-order question therefore remains specific to the boot-logo path until the original `bowtie.raw` is recovered.
