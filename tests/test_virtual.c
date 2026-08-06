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
    unsigned long byte_writes;
    int vpp_seen;
    wl_u16 pending_address;
    wl_u8 pending_data;
    int busy_reads;
    int busy_forever;
    int corrupt_commit;
    int require_sdp;
    int sdp_state;
    unsigned long sdp_commands;
    unsigned long delay_us;
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
        if (!(old & WL_D0) && (value & WL_D0)) {
            v->address_shift = (v->address_shift << 1) |
                               ((value & WL_D1) ? 1UL : 0UL);
            v->address_bits++;
            if (v->address_bits == 24) {
                v->address = (wl_u16)(v->address_shift & 0x1fffUL);
                v->address_bits = 0;
                v->address_shift = 0;
            }
        }

        if (!(old & WL_D1) && (value & WL_D1) && (value & WL_D2)) {
            if ((v->busy_reads || v->busy_forever) &&
                v->address == v->pending_address) {
                v->read_shift = v->pending_data ^ 0x80U;
                if (!v->busy_forever) {
                    v->busy_reads--;
                    if (!v->busy_reads)
                        v->rom[v->pending_address] = v->corrupt_commit ?
                            (wl_u8)(v->pending_data ^ 0x01U) : v->pending_data;
                }
            } else {
                v->read_shift = v->rom[v->address];
            }
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
    wl_u8 old;
    v = (struct virtual_willem *)ctx;
    old = v->control;
    v->control = raw_value ^ WL_CTL_XOR;
    v->control_writes++;
    if (v->control & WL_CTL_VPP) v->vpp_seen = 1;
    if (!(old & WL_CTL_WE) && (v->control & WL_CTL_WE) &&
        (v->control & WL_CTL_VCC) && !(v->control & WL_CTL_MUX)) {
        if (v->require_sdp && v->sdp_state == 0 &&
            v->address == 0x1555U && v->data == 0xaaU) {
            v->sdp_state = 1;
            v->sdp_commands++;
            return;
        }
        if (v->require_sdp && v->sdp_state == 1 &&
            v->address == 0x0aaaU && v->data == 0x55U) {
            v->sdp_state = 2;
            v->sdp_commands++;
            return;
        }
        if (v->require_sdp && v->sdp_state == 2 &&
            v->address == 0x1555U && v->data == 0xa0U) {
            v->sdp_state = 3;
            v->sdp_commands++;
            return;
        }
        if (v->require_sdp && v->sdp_state != 3) {
            v->sdp_state = 0;
            return;
        }
        v->sdp_state = 0;
        v->pending_address = v->address;
        v->pending_data = v->data;
        v->busy_reads = 3;
        v->byte_writes++;
    }
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
    struct virtual_willem *v;
    v = (struct virtual_willem *)ctx;
    v->delay_us += (unsigned long)usec;
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

    v.status_reads = 0;
    wl_begin_28c64_read(&wl);
    for (address = 0; address < ROM_SIZE; address++) {
        actual = wl_read_byte(&wl, address);
        if (actual != pattern(address)) {
            fprintf(stderr, "28C64 mismatch at %04x: got %02x expected %02x\n",
                    address, actual, pattern(address));
            return 1;
        }
    }
    wl_end_read(&wl);
    if (v.status_reads != ROM_SIZE * 8UL) return 1;
    printf("virtual 28C64 read passed: %u bytes, %lu status reads\n",
           ROM_SIZE, v.status_reads);

    /* Runtime timing tables must drive every named read dimension through the
       portable core, including the device power-stabilization delay. */
    v.delay_us = 0;
    wl_set_read_timing(&wl, 2, 3, 4, 5, 7);
    wl_begin_2764_read(&wl);
    actual = wl_read_byte(&wl, 0x0123U);
    if (actual != pattern(0x0123U) || v.delay_us != 7097UL) {
        fprintf(stderr, "profile timing mismatch: data=%02x delay=%lu\n",
                actual, v.delay_us);
        return 1;
    }
    wl_end_read(&wl);
    printf("virtual runtime read profile passed: exact delay accounting\n");

    /* Restore the legacy defaults before exercising write paths, which do not
       consume experimental read timing profiles. */
    wl_set_read_timing(&wl, 0, 1, 1, 1, 0);

    memset(v.rom, 0xff, sizeof(v.rom));
    v.byte_writes = 0;
    wl_begin_28c64_write(&wl);
    for (address = 0; address < ROM_SIZE; address++) {
        if (!wl_write_28c64_byte(&wl, address, pattern(address))) {
            fprintf(stderr, "28C64 write failed at %04x: stored=%02x writes=%lu shifted=%04x\n",
                    address, v.rom[address], v.byte_writes, v.address);
            return 1;
        }
    }
    wl_end_read(&wl);
    if (v.byte_writes != ROM_SIZE) {
        fprintf(stderr, "wrong 28C64 byte write count: %lu\n", v.byte_writes);
        return 1;
    }
    for (address = 0; address < ROM_SIZE; address++)
        if (v.rom[address] != pattern(address)) return 1;
    if (v.control & (WL_CTL_VPP | WL_CTL_VCC)) return 1;
    if (v.vpp_seen) {
        fprintf(stderr, "unsafe VPP request during virtual tests\n");
        return 1;
    }
    printf("virtual 28C64 write passed: %lu byte writes, VPP never requested\n",
           v.byte_writes);

    /* A protected part rejects an ordinary byte write but accepts the exact
       Microchip three-load SDP prefix followed by the data byte. */
    v.require_sdp = 1;
    v.sdp_state = 0;
    v.sdp_commands = 0;
    v.byte_writes = 0;
    v.rom[0x0123U] = 0x00U;
    wl_begin_28c64_write(&wl);
    if (wl_write_28c64_byte(&wl, 0x0123U, 0x5aU)) {
        fprintf(stderr, "SDP-protected device accepted ordinary write\n");
        return 1;
    }
    if (!wl_write_28c64_sdp_byte(&wl, 0x0123U, 0x5aU)) {
        fprintf(stderr, "SDP protected write sequence failed\n");
        return 1;
    }
    wl_end_read(&wl);
    if (v.rom[0x0123U] != 0x5aU || v.byte_writes != 1UL ||
        v.sdp_commands != 3UL) {
        fprintf(stderr, "wrong SDP sequence result: data=%02x writes=%lu commands=%lu\n",
                v.rom[0x0123U], v.byte_writes, v.sdp_commands);
        return 1;
    }
    printf("virtual SDP rejection and protected write passed\n");
    v.require_sdp = 0;

    /* A device that never leaves its self-timed cycle must fail after the
       complete polling allowance, not loop forever or report success. */
    v.busy_forever = 1;
    v.delay_us = 0;
    wl_begin_28c64_write(&wl);
    v.delay_us = 0;
    if (wl_write_28c64_byte(&wl, 0x0123U, 0x5aU)) {
        fprintf(stderr, "permanently busy 28C64 incorrectly passed\n");
        return 1;
    }
    if (v.delay_us < 20000UL) {
        fprintf(stderr, "28C64 timeout too short: %lu us\n", v.delay_us);
        return 1;
    }
    wl_end_read(&wl);
    if (v.control & (WL_CTL_VPP | WL_CTL_VCC)) {
        fprintf(stderr, "unsafe state after 28C64 timeout: %02x\n", v.control);
        return 1;
    }
    v.busy_forever = 0;

    /* D7 completion alone is insufficient: the returned byte must match in
       full. Simulate a device that completes with one corrupt low bit. */
    v.corrupt_commit = 1;
    wl_begin_28c64_write(&wl);
    if (wl_write_28c64_byte(&wl, 0x0456U, 0xa4U)) {
        fprintf(stderr, "corrupt 28C64 completion incorrectly passed\n");
        return 1;
    }
    wl_end_read(&wl);
    if (v.control & (WL_CTL_VPP | WL_CTL_VCC)) {
        fprintf(stderr, "unsafe state after corrupt completion: %02x\n",
                v.control);
        return 1;
    }
    printf("virtual 28C64 failure paths passed: timeout and corrupt data\n");
    return 0;
}
