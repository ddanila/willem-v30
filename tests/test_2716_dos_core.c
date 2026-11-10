/* Small enough for DOS tiny model: exercise the actual bcc-generated calls.
   No hardware I/O. This catches 16/32-bit K&R argument-width mismatches. */
#include <stdio.h>
#include <string.h>
#include "willem.h"
static unsigned data, control, clocks, address, expected, readbit;
static unsigned pulses, errors, probe_mode, alias_mask;
static unsigned long shift;
static void data_write(ctx, value)
void *ctx;
int value;
{
    unsigned old = data;
    data = value;
    if ((control & 2) && !(old & 1) && (data & 1)) {
        shift = (shift << 1) | ((data & 2) ? 1UL : 0UL);
        clocks++;
    }
}
static void control_write(ctx, value)
void *ctx;
int value;
{
    unsigned old = control;
    control = value ^ 11;
    if ((old & 2) && !(control & 2) && clocks) {
        if (clocks != 24) errors++;
        address = (unsigned)(shift & 0x7ffUL);
        clocks = 0;
        shift = 0;
    }
}
static wl_u8 status_read(ctx)
void *ctx;
{
    unsigned bit = (0x96U >> (7 - (readbit++ & 7))) & 1;
    if (probe_mode) {
        bit = ((address & ~alias_mask) == 0 ? 0x96U : 0xffU);
        bit = (bit >> (7 - ((readbit - 1) & 7))) & 1;
    } else if (address != expected) errors++;
    return bit ? 0 : 0x40;
}
static void delay(ctx, usec)
void *ctx;
int usec;
{
}
static void pulse(ctx)
void *ctx;
{
    pulses++;
    if (address != expected || control != 5 || data != 0x96)
        errors++;
}
int main()
{
    struct wl_io io;
    struct willem wl;
    wl_u8 verified;
    wl_u16 bad;
    unsigned i;
    memset(&io, 0, sizeof(io));
    io.data_write = data_write;
    io.control_write = control_write;
    io.status_read = status_read;
    io.delay_us = delay;
    io.pulse_2716 = pulse;
    wl_init(&wl, &io);
    wl_begin_2716_program(&wl);
    for (i = 0; i < 2048; i++) {
        expected = i;
        if (!wl_program_2716_byte(&wl, i, 0x96, &verified)) errors++;
    }
    wl_vpp(&wl, 0);
    probe_mode = 1;
    if (!wl_check_2716_probe(&wl, 0U, 0x96, &bad, &verified)) errors++;
    alias_mask = 0x40;
    if (wl_check_2716_probe(&wl, 0U, 0x96, &bad, &verified) ||
        bad != 0x40 || verified != 0x96) errors++;
    wl_end_read(&wl);
    if (pulses != 2048 || (control & 5)) errors++;
    printf("DOS core address test: errors=%u pulses=%u\n", errors, pulses);
    return errors ? 1 : 0;
}
