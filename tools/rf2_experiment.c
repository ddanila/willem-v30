/* Bounded experiments on an explicitly sacrificial 2716/RF2.
   W only accepts a currently FF byte, one fixed 50 ms pulse, no retry.
   R orders: 0 linear, 1 reverse, 2 permuted, 3 CE disabled, 4 ASM shifter.
   F fills only the A9-zero half from a mirror-consistent image, preserving
   existing matching bytes. Only FF cells can receive a single pulse.
   Delay is added after each C OUT (not inside the ASM address shifter).
   All commands finish with VPP/VCC off. No changes to normal writer timing. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "willem.h"
extern void dos_outb(unsigned port, unsigned value);
extern unsigned dos_inb(unsigned port);
extern void dos_wait_us(unsigned usec);
extern void dos_2716_pulse(unsigned base);
extern unsigned dos_shift_address(unsigned base, unsigned long address, unsigned data, unsigned control);
static unsigned pace, bad_argument;
static unsigned char buffer[2048];
static void data_out(ctx, value)
void *ctx; int value;
{ dos_outb(0x378U, (unsigned)value); if (pace) dos_wait_us(pace); }
static void control_out(ctx, value)
void *ctx; int value;
{ dos_outb(0x37aU, (unsigned)value); if (pace) dos_wait_us(pace); }
static wl_u8 status_in(ctx)
void *ctx;
{ return (wl_u8)dos_inb(0x379U); }
static void delay(ctx, usec)
void *ctx; int usec;
{ dos_wait_us((unsigned)usec); }
static void pulse(ctx)
void *ctx;
{ dos_2716_pulse(0x378U); }
static unsigned number(s)
char *s;
{
    unsigned value = 0, digit;
    if (!*s) bad_argument = 1;
    while (*s) {
        if (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (*s >= 'a' && *s <= 'f') digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') digit = *s - 'A' + 10;
        else { bad_argument = 1; return 0; }
        if (value > 0xfffU) { bad_argument = 1; return 0; }
        value = value * 16U + digit;
        s++;
    }
    return value;
}
int main(argc, argv)
int argc; char **argv;
{
    struct wl_io io;
    struct willem wl;
    unsigned i, address, value, order, count, skipped;
    wl_u8 high, low;
    int result;
    FILE *file;
    if (argc != 5 || (strcmp(argv[1], "R") && strcmp(argv[1], "W") && strcmp(argv[1], "F") && strcmp(argv[1], "A"))) {
        puts("RF2EXP R file pace order | W addr byte pace | F image pace 200 | A addr seconds pace (hex)");
        return 1;
    }
    result = 1;
    if (!strcmp(argv[1], "R")) {
        pace = number(argv[3]); order = number(argv[4]);
        if (pace > 1000U || order > 4U) return 1;
    } else if (!strcmp(argv[1], "A")) {
        address = number(argv[2]); value = number(argv[3]); pace = number(argv[4]);
        if (address >= 2048U || value == 0 || value > 60U || pace > 1000U) return 1;
    } else {
        if (!strcmp(argv[1], "F")) {
            pace = number(argv[3]);
            if (number(argv[4]) != 0x200U || pace > 1000U) return 1;
        } else {
            address = number(argv[2]); value = number(argv[3]); pace = number(argv[4]);
            if (address >= 2048U || value >= 255U || pace > 1000U) return 1;
        }
        file = fopen("RF2.OK", "rb");
        if (!file) { puts("Missing RF2.OK"); return 1; }
        if (!fgets((char *)buffer, 48, file)) { fclose(file); return 1; }
        fclose(file);
        if (strncmp((char *)buffer, "WILLEM-2716-5V-25V-GATE-1", 23)) return 1;
    }
    if (!strcmp(argv[1], "F")) {
        file = fopen(argv[2], "rb");
        if (!file) return 1;
        count = fread(buffer, 1, 2048, file);
        i = fgetc(file);
        fclose(file);
        if (count != 2048U || i != (unsigned)EOF) return 1;
        for (i = 0; i < 2048U; i++)
            if (buffer[i] != buffer[i ^ 0x200U]) return 1;
    }
    if (bad_argument) return 1;
    memset(&io, 0, sizeof(io));
    io.data_write = data_out; io.control_write = control_out;
    io.status_read = status_in; io.delay_us = delay; io.pulse_2716 = pulse;
    wl_init(&wl, &io);
    wl_set_read_timing(&wl, 20U, 20U, 20U, 20U, 10U);
    wl_begin_2716_read(&wl);
    if (!strcmp(argv[1], "A")) {
        wl_oe(&wl, 1);
        wl_set_address(&wl, (wl_u32)address, WL_ADDR_FIRST_BIT);
        wl_set_data(&wl, 0);
        printf("ADDRESS HOLD %04X; VPP OFF, VCC ON; A9 expected %u; approx %u seconds\n",
               address, (address >> 9) & 1U, value);
        for (i = 0; i < value * 20U; i++) dos_wait_us(50000U);
        result = 0;
    } else if (!strcmp(argv[1], "F")) {
        count = skipped = 0;
        for (i = 0; i < 2048U; i++) {
            low = wl_read_byte(&wl, i);
            if (low != 0xffU && low != buffer[i]) {
                printf("REFUSED: incompatible byte %04X=%02X wanted=%02X\n", i, low, buffer[i]);
                goto safe;
            }
        }
        wl_begin_2716_program(&wl);
        for (i = 0; i < 2048U; i++) {
            if (i & 0x200U) continue;
            low = wl_read_byte(&wl, i);
            if (low == buffer[i]) { skipped++; continue; }
            if (low != 0xffU) { printf("ABORT: nonblank %04X\n",i); goto safe; }
            if (!wl_program_2716_byte(&wl, i, (int)buffer[i], &high)) {
                printf("ABORT: cell %04X high=%02X wanted=%02X\n",i,high,buffer[i]); goto safe;
            }
            count++;
            if (!(count & 127U)) printf("Fill pulses=%u last=%04X\n",count,i);
        }
        wl_vpp(&wl, 0); dos_wait_us(10000U);
        result = 0;
        for (i = 0; i < 2048U; i++) {
            low = wl_read_byte(&wl, i);
            if (low != buffer[i]) {
                printf("VERIFY mismatch %04X=%02X wanted=%02X\n",i,low,buffer[i]);
                result = 2; break;
            }
        }
        printf("Fill pulses=%u skipped=%u result=%d\n", count,skipped,result);
    } else if (!strcmp(argv[1], "R")) {
        if (order == 3U) wl_we(&wl, 1); /* CE disabled, VPP remains off. */
        for (i = 0; i < 2048U; i++) {
            address = order == 1U ? 2047U-i :
                      (order == 2U ? (i*73U+17U)&2047U : i);
            if (order == 4U) {
                /* Independent, already qualified assembly address shifter. */
                wl.data = (wl_u8)dos_shift_address(0x378U, (wl_u32)address,
                                                  (unsigned)wl.data, (unsigned)wl.control);
                wl.control |= WL_CTL_MUX;
                dos_wait_us(20U);
                wl_oe(&wl, 1); wl_oe(&wl, 0); dos_wait_us(20U);
                buffer[address] = wl_get_data(&wl); wl_oe(&wl, 1);
            } else buffer[address] = wl_read_byte(&wl, address);
        }
        result = 0;
    } else {
        low = wl_read_byte(&wl, address);
        printf("Before %04X=%02X pace=%u us\n", address, low, pace);
        if (low == 0xffU) {
            wl_begin_2716_program(&wl);
            result = wl_program_2716_byte(&wl, address, (int)value, &high) ? 0 : 2;
            wl_vpp(&wl, 0);
            dos_wait_us(10000U);
            low = wl_read_byte(&wl, address);
            printf("One pulse %04X <- %02X: high-VPP=%02X normal=%02X\n",
                   address, value, high, low);
            if (low != value) result = 2;
        } else puts("REFUSED: target byte is not FF; no pulse");
    }
safe:
    wl_end_read(&wl);
    puts("SAFE OFF: VPP off, VCC off");
    if (!strcmp(argv[1], "R")) {
        file = fopen(argv[2], "wb");
        if (!file) return 1;
        if (fwrite(buffer, 1, 2048, file) != 2048U) result = 1;
        fclose(file);
        printf("Read 2048 bytes; pace=%u us; order=%u; result=%d\n", pace, order, result);
    }
    return result;
}
