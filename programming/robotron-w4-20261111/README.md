# Robotron1715M/W W4 4.0.0 — Fujitsu MBM2732A-30

Prepared from the qualified inventory candidate at revision `e748f44`.
`W4-2K.BIN` is the exact original 2048-byte ROM; `R1715W4.BIN` concatenates
it with itself for a 4096-byte 2732A. Both halves contain the full original
firmware, fingerprint and CRC. This is W4 firmware **4.0.0**, not the W2
image previously burned and labelled **1715MNETv1**.

Source: [inventory W4 candidate](https://github.com/ddanila/vintage-computer-inventory/tree/e748f44/items/PC001-robotron-1715m/evidence/20261111-w4-candidate),
[Ravi source](https://github.com/ddanila/ravi/tree/3de77aca760155c1c8f6b0f2bcde336cde785523).
Build ID `ac6dcf19b54cb668`; original ROM CRC16 `2ef5`. The inventory records
44 passing ROM tests and CP/M cold-boot, file-copy and warm-boot qualification
at all supported rates. Physical target-machine acceptance remains pending.
Use a W4-capable Ravi host: the firmware scans **62400, 41600 and 9600 8N1/x16**,
requires nonce confirmation, and locks the negotiated rate until release.

The installed chip was identified by the owner as **MBM2732A-30**. Fujitsu
specifies 5 V VCC, 21 V VPP and a 50 ms active-low CE programming pulse,
matching our existing PG2732A.COM implementation. The programmer retained
the previously measured 21 V setup. No new loaded rail measurement is claimed.
The existing gate token and DOS program name remain unchanged; the chip's
actual manufacturer and marking are recorded in `manifest.json`.

[Fujitsu programming specification](https://deramp.com/downloads/mfe_archive/050-Component%20Specifications/Fujitsu/Memory%20Components/1982_Fujitsu_Memory_Data_Book.pdf#page=140).

Initial independent read: all 4096 bytes FF, execution857856748, exit0,
VPP/VCC off. The blank binary remains in local DOSRAVI sessions.

## Completed programming

The unchanged writer passed its full blank scan and first-byte alias check,
then programmed all 3742 non-FF bytes with one 50 ms pulse each and verified
all4096 bytes after power-cycling. Execution2637388255 exited0.

Independent RD2732.COM readback, execution1630888582, also exited0 and matched
the complete4 KiB image. Both2 KiB halves independently match the qualified
W4 source. **VPP and VCC are off.** Owner-applied physical chip label: **1715MNETv4**.

Burn/readback SHA-256:
`53222fd157d93b83c381df78c707d8a047b140b098368c307cd7c388a79bd97a`.
The 2732A burn is complete; physical boot/RESET/serial acceptance in PC001
remains pending. No other EPROM was altered.
