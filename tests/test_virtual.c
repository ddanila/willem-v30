#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "willem.h"

#define ROM_SIZE 8192

struct virtual_willem {
    wl_u8 rom[ROM_SIZE];
    wl_u8 data;
    wl_u8 control;
    wl_u8 read_shift;
    wl_u8 read_mask;
    wl_u32 address_shift;
    wl_u16 address_bits;
    wl_u16 address;
    int read_loaded;
    unsigned long data_writes;
    unsigned long control_writes;
    unsigned long status_reads;
};

static void virtual_data_write(ctx, value)
void *ctx;
int value;
{
    struct virtual_willem *v;
    wl_u8 old;

    v = (struct virtual_willem *)ctx;
    old = v->data;
    v->data = (wl_u8)value;
    v->data_writes++;

    if (v->control & WL_CTL_MUX) {
        if ((old & WL_D0) && !(value & WL_D0)) {
            v->address_shift = (v->address_shift << 1) |
                               ((value & WL_D1) ? 1UL : 0UL);
            v->address_bits++;
            if (v->address_bits == 13) {
                v->address = (wl_u16)(v->address_shift & 0x1fffUL);
                v->address_bits = 0;
                v->address_shift = 0;
            }
        }

        if (!(old & WL_D1) && (value & WL_D1) && (value & WL_D2)) {
            v->read_shift = v->rom[v->address];
            v->read_mask = 0x80;
            v->read_loaded = 1;
        }

        if (!(old & WL_D2) && (value & WL_D2) && v->read_loaded) {
            if (v->read_loaded == 1) v->read_loaded = 2;
            else if (v->read_mask) v->read_mask >>= 1;
        }
    }
}

static void virtual_control_write(ctx, raw_value)
void *ctx;
int raw_value;
{
    struct virtual_willem *v;
    v = (struct virtual_willem *)ctx;
    v->control = raw_value ^ WL_CTL_XOR;
    v->control_writes++;
}

static wl_u8 virtual_status_read(ctx)
void *ctx;
{
    struct virtual_willem *v;
    v = (struct virtual_willem *)ctx;
    v->status_reads++;
    if (v->read_mask && (v->read_shift & v->read_mask)) return 0x00;
    return WL_ST_ACK;
}

static void virtual_delay(ctx, usec)
void *ctx;
int usec;
{
    (void)ctx;
    (void)usec;
}

static wl_u8 pattern(address)
wl_u16 address;
{
    return (wl_u8)(((address * 73U) ^ (address >> 3) ^ 0xa5U) & 0xffU);
}

int main()
{
    struct virtual_willem v;
    struct willem wl;
    struct wl_io io;
    wl_u16 address;
    wl_u8 actual;

    memset(&v, 0, sizeof(v));
    for (address = 0; address < ROM_SIZE; address++) v.rom[address] = pattern(address);

    io.ctx = &v;
    io.data_write = virtual_data_write;
    io.control_write = virtual_control_write;
    io.status_read = virtual_status_read;
    io.delay_us = virtual_delay;

    wl_init(&wl, &io);
    wl_begin_2764_read(&wl);
    for (address = 0; address < ROM_SIZE; address++) {
        actual = wl_read_byte(&wl, address);
        if (actual != pattern(address)) {
            fprintf(stderr, "mismatch at %04x: got %02x expected %02x\n",
                    address, actual, pattern(address));
            return 1;
        }
    }
    wl_end_read(&wl);

    if (v.status_reads != ROM_SIZE * 8UL) {
        fprintf(stderr, "wrong status read count: %lu\n", v.status_reads);
        return 1;
    }
    if (v.control & (WL_CTL_VPP | WL_CTL_VCC)) {
        fprintf(stderr, "unsafe final power state: %02x\n", v.control);
        return 1;
    }

    printf("virtual 2764 read passed: %u bytes, %lu status reads\n",
           ROM_SIZE, v.status_reads);
    return 0;
}
