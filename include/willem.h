#ifndef WILLEM_H
#define WILLEM_H

typedef unsigned char wl_u8;
typedef unsigned int wl_u16;
typedef unsigned long wl_u32;

/* Logical DB-25 pin masks, matching Geepro's terminology. */
#define WL_D0       0x01
#define WL_D1       0x02
#define WL_D2       0x04
#define WL_CTL_VPP  0x01 /* DB-25 pin 1, physically inverted */
#define WL_CTL_MUX  0x02 /* DB-25 pin 14, physically inverted */
#define WL_CTL_VCC  0x04 /* DB-25 pin 16 */
#define WL_CTL_WE   0x08 /* DB-25 pin 17, physically inverted */
#define WL_CTL_XOR  0x0b /* PC control-register inversion */
#define WL_ST_ACK   0x40 /* DB-25 pin 10 */
#define WL_ADDR_FIRST_BIT 0x800000UL /* Geepro's complete 24-bit chain */

struct wl_io {
    void *ctx;
    void (*data_write)(void *ctx, int value);
    void (*control_write)(void *ctx, int raw_value);
    wl_u8 (*status_read)(void *ctx);
    void (*delay_us)(void *ctx, int usec);
};

struct willem {
    struct wl_io io;
    wl_u8 data;
    wl_u8 control; /* logical connector levels, before WL_CTL_XOR */
};

void wl_init(struct willem *wl, struct wl_io *io);
void wl_safe(struct willem *wl);
void wl_vcc(struct willem *wl, int on);
void wl_vpp(struct willem *wl, int on);
void wl_oe(struct willem *wl, int asserted);
void wl_we(struct willem *wl, int asserted);
void wl_set_address(struct willem *wl, wl_u32 address, wl_u32 first_bit);
void wl_set_data(struct willem *wl, int value);
wl_u8 wl_get_data(struct willem *wl);
void wl_begin_2764_read(struct willem *wl);
void wl_begin_28c64_read(struct willem *wl);
void wl_begin_28c64_write(struct willem *wl);
void wl_end_read(struct willem *wl);
wl_u8 wl_read_byte(struct willem *wl, wl_u16 address);
int wl_write_28c64_byte(struct willem *wl, wl_u16 address, int value);

#endif
