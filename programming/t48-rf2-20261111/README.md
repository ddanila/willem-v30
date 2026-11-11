# First direct-USB T48 RF2 read — 2026-11-11

Owner installed an RF2 in a newly connected T48 and requested reading directly
from this Linux machine. No DOSRAVI connection, firmware update, programming
or erase operation was performed.

Two separate `M2716@DIP24` reads returned identical 2048-byte all-FF images,
with zero exit status and 65.2 ms reported read time each. This is consistent
with a blank chip, but does not establish socket contact or address-line
independence. A requested profile pin test reported **not supported**. Do not
infer that this is one of the previously programmed RF2 specimens; the owner
has not identified its prior history or full marking.

The all-FF files are kept only in local diagnostic storage:
`/home/ddanila/fun/minipro-sessions/t48-rf2-20261111/`.
The manifest records hashes and results; no blank firmware is archived here.

## Software and connection

Built unmodified upstream minipro 0.7.4, commit
`3e2f4911fa86f72a57ae0ace777342e75da5e7cc`, from
https://gitlab.com/DavidGriffith/minipro into `/home/ddanila/fun/minipro`.
The detected device is T48 firmware `00.1.03 (0x103)`. USB ID `a466:0a53`
is generically labelled TL866II+ by lsusb; minipro's device query identifies
T48. Existing desktop USB ACLs give the user read/write access.

minipro warns that T48 support is incomplete and expects firmware `0x127`.
The read operations nevertheless completed successfully. No compatibility
claim beyond these two reads is made, and no firmware update was attempted.

The system lacked libusb development headers. Downloaded Ubuntu's
`libusb-1.0-0-dev` package and extracted it under `/tmp/minipro-build-deps/root`,
then linked against the already installed runtime library, without sudo:

```
make -j4 libusb_CFLAGS=-I/tmp/minipro-build-deps/root/usr/include/libusb-1.0 libusb_LIBS=-l:libusb-1.0.so.0 SHARE_INSTDIR=/home/ddanila/fun/minipro
```

Read command (choose a new output filename for each acquisition):

```
/home/ddanila/fun/minipro/minipro -p 'M2716@DIP24' -x -c code -r OUTPUT.bin
```

`-x` skips chip-ID probing. The selected standard single-supply 2716 profile
has 5 V read/verify VCC. This is not the three-supply TI TMS2716 profile.
The programming defaults shown by `-d` are not qualified for RF2 writing;
this session only used the read path. Do not transfer Willem ZIF contact
numbers or jumper instructions to the T48.
