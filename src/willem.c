#include "willem.h"

static void control_commit(wl)
struct willem *wl;
{
    wl->io.control_write(wl->io.ctx, wl->control ^ WL_CTL_XOR);
}

static void control_bit(wl, bit, on)
struct willem *wl;
wl_u8 bit;
int on;
{
    if (on) wl->control |= bit;
    else wl->control &= (wl_u8)~bit;
    control_commit(wl);
}

static void data_commit(wl)
struct willem *wl;
{
    wl->io.data_write(wl->io.ctx, wl->data);
}

static void data_bit(wl, bit, on)
struct willem *wl;
int bit;
int on;
{
    if (on) wl->data |= bit;
    else wl->data &= (wl_u8)~bit;
    data_commit(wl);
}

static void delay_us(wl, usec)
struct willem *wl;
int usec;
{
    if (wl->io.delay_us) wl->io.delay_us(wl->io.ctx, usec);
}

void wl_init(wl, io)
struct willem *wl;
struct wl_io *io;
{
    wl->io = *io;
    wl->data = 0;
    wl->control = 0;
    data_commit(wl);
    control_commit(wl);
}

void wl_safe(wl)
struct willem *wl;
{
    /* Data low; VPP, VCC and active-low socket controls deasserted. */
    wl->data = 0;
    wl->control = 0;
    data_commit(wl);
    control_commit(wl);
}

void wl_vcc(wl, on)
struct willem *wl;
int on;
{
    control_bit(wl, WL_CTL_VCC, on);
}

void wl_vpp(wl, on)
struct willem *wl;
int on;
{
    control_bit(wl, WL_CTL_VPP, on);
}

void wl_oe(wl, asserted)
struct willem *wl;
int asserted;
{
    /* Geepro: asserted OE clears logical pin 14. */
    control_bit(wl, WL_CTL_MUX, !asserted);
}

void wl_we(wl, asserted)
struct willem *wl;
int asserted;
{
    control_bit(wl, WL_CTL_WE, asserted);
}

void wl_set_address(wl, address, first_bit)
struct willem *wl;
wl_u32 address;
wl_u32 first_bit;
{
    wl_u32 mask;

    /* Route D0/D1 to the cascaded address registers. */
    control_bit(wl, WL_CTL_MUX, 1);
    wl->data &= (wl_u8)~(WL_D0 | WL_D1);
    data_commit(wl);

    mask = first_bit;
    while (mask) {
        data_bit(wl, WL_D1, (address & mask) != 0);
        data_bit(wl, WL_D0, 1);
        data_bit(wl, WL_D0, 0);
        mask >>= 1;
    }

    /* Return the multiplexor to the ROM data path. */
    control_bit(wl, WL_CTL_MUX, 0);
}

void wl_set_data(wl, value)
struct willem *wl;
int value;
{
    control_bit(wl, WL_CTL_MUX, 0);
    wl->data = (wl_u8)value;
    data_commit(wl);
}

wl_u8 wl_get_data(wl)
struct willem *wl;
{
    wl_u16 bit;
    wl_u8 value;

    value = 0;
    control_bit(wl, WL_CTL_MUX, 1);
    data_bit(wl, WL_D2, 1);
    data_bit(wl, WL_D1, 0);
    delay_us(wl, 1);
    data_bit(wl, WL_D1, 1);
    delay_us(wl, 1);
    data_bit(wl, WL_D2, 0);
    delay_us(wl, 1);
    data_bit(wl, WL_D2, 1);
    data_bit(wl, WL_D1, 0);

    for (bit = 0x80; bit != 0; bit >>= 1) {
        delay_us(wl, 1);
        /* The Willem input chain and PC ACK input are active-low. */
        if (!(wl->io.status_read(wl->io.ctx) & WL_ST_ACK))
            value |= (wl_u8)bit;
        data_bit(wl, WL_D2, 0);
        delay_us(wl, 1);
        data_bit(wl, WL_D2, 1);
    }
    return value;
}

wl_u8 wl_read_byte(wl, address)
struct willem *wl;
wl_u16 address;
{
    wl_u8 value;
    wl_set_address(wl, (wl_u32)address, 0x1000UL);
    wl_oe(wl, 0);
    delay_us(wl, 1);
    value = wl_get_data(wl);
    wl_oe(wl, 1);
    return value;
}

void wl_begin_2764_read(wl)
struct willem *wl;
{
    wl_vpp(wl, 0);
    wl_vcc(wl, 1);
    wl_oe(wl, 0);
    wl_we(wl, 1); /* DIP routing makes pin 17 the 2764 CE control. */
    delay_us(wl, 5000);
}

void wl_begin_28c64_read(wl)
struct willem *wl;
{
    wl_vpp(wl, 0);
    wl_we(wl, 1);
    wl_oe(wl, 1);
    wl_vcc(wl, 1);
    /* Geepro allows 200 ms after enabling 5 V for the 2864 family. */
    delay_us(wl, 50000);
    delay_us(wl, 50000);
    delay_us(wl, 50000);
    delay_us(wl, 50000);
    wl_oe(wl, 0);
    wl_we(wl, 1);
}

void wl_end_read(wl)
struct willem *wl;
{
    wl_vpp(wl, 0);
    wl_oe(wl, 0);
    wl_we(wl, 0);
    wl_vcc(wl, 0);
    wl_set_address(wl, 0, 0x1000UL);
    wl_set_data(wl, 0);
}
