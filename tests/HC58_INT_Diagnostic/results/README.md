# Raw HC58 INT Diagnostic Logs

These are the original Tera Term text logs compressed with deterministic gzip (`gzip -n`).

To inspect on Linux/macOS:

```bash
gzip -dc 2026-10-23_full_20x3.txt.gz | less
gzip -dc 2026-10-23_irq_only_100.txt.gz | less
```

On Windows, 7-Zip can extract the `.txt` files.

## Files

### `2026-10-23_full_20x3.txt.gz`

Contains the complete automatic comparison:

- 20 MASKED_POLL requests
- 20 IRQ_POLL requests
- 20 IRQ_ONLY requests

Original text:

- bytes: 36,366
- lines: 290
- SHA-256: `53e5263e002020cf282a9796cfd1e88b192a1440c4772018ea10dfd541734b48`

Compressed file:

- SHA-256: `8226fb94a6903bd199143d66f5035d2422acbb4da645e277cdcb5f9499b5d9f4`

### `2026-10-23_irq_only_100.txt.gz`

Contains the complete automatic comparison followed by the separate 100-request IRQ-only run.

Original text:

- bytes: 94,458
- lines: 722
- SHA-256: `775fb031502ac266ff954c5989b3a8504713ebd2eb29bccb24e834bc68477c17`

Compressed file:

- SHA-256: `5d501b6d61a9bf321157b82bfb63f9031c0f13a74aec4cfa5955843be496bf9d`
