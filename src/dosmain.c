#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "willem.h"

#define ROM_SIZE 8192
#define RF5_SIZE 2048
#define LOG_NAME "WILLEM.LOG"
#define TRACE_NAME "WTRACE.BIN"
#define WRITE_GATE_NAME "WRITE.OK"
#define WILLEM_VERSION "0.1.0-dev"
#ifndef WILLEM_BUILD_ID
#define WILLEM_BUILD_ID "dosravi-rf5-read-v1"
#endif

#define ACTION_READ 1
#define ACTION_BLANK 2
#define ACTION_VERIFY 3
#define ACTION_WRITE 4
#define ACTION_DIAG 5
#define DEVICE_2764 1
#define DEVICE_28C64 2
#define DEVICE_RF5 3

extern void dos_outb(unsigned port, unsigned value);
extern unsigned dos_inb(unsigned port);
extern void dos_wait_us(unsigned usec);
extern void dos_datetime(unsigned *fields);
extern void dos_sdp_write(unsigned base, unsigned address, unsigned value);
extern unsigned long dos_bios_ticks(void);

struct read_profile {
    char *name;
    unsigned address_setup_us;
    unsigned oe_settle_us;
    unsigned input_latch_us;
    unsigned input_clock_us;
    unsigned power_2764_ms;
    unsigned power_28c64_ms;
};

/* Adjacent experimental profiles change one timing dimension. The legacy
   entry exactly preserves the pre-profile read path for comparison. */
static struct read_profile read_profiles[] = {
    {"legacy",       0, 1, 1, 1, 5, 200},
    {"conservative", 4, 4, 4, 4, 5, 200},
    {"address2",     2, 4, 4, 4, 5, 200},
    {"oe2",          2, 2, 4, 4, 5, 200},
    {"latch2",       2, 2, 2, 4, 5, 200},
    {"balanced",     2, 2, 2, 2, 5, 200},
    {"address1",     1, 2, 2, 2, 5, 200},
    {"oe1",          1, 1, 2, 2, 5, 200},
    {"latch1",       1, 1, 1, 2, 5, 200},
    {"fast",         1, 1, 1, 1, 5, 200},
    {"powerfast",    1, 1, 1, 1, 4, 150}
};

struct dos_context {
    unsigned base;
    FILE *trace;
    unsigned long sequence;
};

static FILE *log_file;
static unsigned char rom_buffer[ROM_SIZE];

static FILE *open_append(name, binary)
char *name;
int binary;
{
    FILE *file;
    file = fopen(name, binary ? "r+b" : "r+");
    if (file) {
        if (fseek(file, 0L, SEEK_END)) {
            fclose(file);
            return 0;
        }
        return file;
    }
    return fopen(name, binary ? "w+b" : "w+");
}

static void timestamp(out)
char *out;
{
    unsigned d[7];
    dos_datetime(d);
    sprintf(out, "%04u-%02u-%02u %02u:%02u:%02u.%02u",
            d[0], d[1], d[2], d[3], d[4], d[5], d[6]);
}

static void logmsg(char *format, ...)
{
    va_list args;
    char stamp[32];
    char message[256];

    va_start(args, format);
    vsprintf(message, format, args);
    va_end(args);
    timestamp(stamp);
    printf("[%s] %s\n", stamp, message);
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", stamp, message);
        fflush(log_file);
    }
}

static void trace_record(ctx, operation, reg, value)
struct dos_context *ctx;
int operation;
int reg;
unsigned value;
{
    unsigned char record[8];
    unsigned long sequence;

    if (!ctx->trace) return;
    sequence = ctx->sequence++;
    record[0] = (unsigned char)(sequence & 0xffUL);
    record[1] = (unsigned char)((sequence >> 8) & 0xffUL);
    record[2] = (unsigned char)((sequence >> 16) & 0xffUL);
    record[3] = (unsigned char)((sequence >> 24) & 0xffUL);
    record[4] = (unsigned char)operation;
    record[5] = (unsigned char)reg;
    record[6] = (unsigned char)(value & 0xffU);
    record[7] = (unsigned char)((value >> 8) & 0xffU);
    fwrite(record, 1, sizeof(record), ctx->trace);
}

static void port_data_write(vctx, value)
void *vctx;
int value;
{
    struct dos_context *ctx = (struct dos_context *)vctx;
    dos_outb(ctx->base, (unsigned)value);
    trace_record(ctx, 'O', 0, (unsigned)value);
}

static void port_control_write(vctx, value)
void *vctx;
int value;
{
    struct dos_context *ctx = (struct dos_context *)vctx;
    dos_outb(ctx->base + 2, (unsigned)value);
    trace_record(ctx, 'O', 2, (unsigned)value);
}

static wl_u8 port_status_read(vctx)
void *vctx;
{
    struct dos_context *ctx = (struct dos_context *)vctx;
    unsigned value = dos_inb(ctx->base + 1);
    trace_record(ctx, 'I', 1, value);
    return (wl_u8)value;
}

static void port_delay(vctx, usec)
void *vctx;
int usec;
{
    struct dos_context *ctx = (struct dos_context *)vctx;
    dos_wait_us((unsigned)usec);
    trace_record(ctx, 'D', 0, (unsigned)usec);
}

static unsigned crc16(data, size)
unsigned char *data;
unsigned size;
{
    unsigned crc, i, bit;
    crc = 0xffffU;
    for (i = 0; i < size; i++) {
        crc ^= (unsigned)data[i] << 8;
        for (bit = 0; bit < 8; bit++)
            crc = (crc & 0x8000U) ? (crc << 1) ^ 0x1021U : crc << 1;
    }
    return crc;
}

static unsigned long crc32(data, size)
unsigned char *data;
unsigned size;
{
    unsigned long crc;
    unsigned i, bit;
    crc = 0xffffffffUL;
    for (i = 0; i < size; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++)
            crc = (crc & 1UL) ? (crc >> 1) ^ 0xedb88320UL : crc >> 1;
    }
    return crc ^ 0xffffffffUL;
}

static unsigned parse_base(text)
char *text;
{
    unsigned value;
    char extra;
    if (!text) return 0x378;
    if (sscanf(text, "%x%c", &value, &extra) != 1) return 0;
    return value;
}

static int same_command(left, right)
char *left;
char *right;
{
    unsigned char a, b;
    while (*left && *right) {
        a = (unsigned char)*left++;
        b = (unsigned char)*right++;
        if (a >= 'a' && a <= 'z') a -= 'a' - 'A';
        if (b >= 'a' && b <= 'z') b -= 'a' - 'A';
        if (a != b) return 0;
    }
    return *left == *right;
}

static char *profile_value(option)
char *option;
{
    char *prefix;
    unsigned char a, b;
    prefix = "/PROFILE:";
    while (*prefix && *option) {
        a = (unsigned char)*option++;
        b = (unsigned char)*prefix++;
        if (a >= 'a' && a <= 'z') a -= 'a' - 'A';
        if (a != b) return 0;
    }
    return *prefix ? 0 : option;
}

static struct read_profile *find_profile(name)
char *name;
{
    unsigned i;
    for (i = 0; i < sizeof(read_profiles) / sizeof(read_profiles[0]); i++)
        if (same_command(name, read_profiles[i].name)) return &read_profiles[i];
    return 0;
}

static int wants_trace(argc, argv)
int argc;
char **argv;
{
    int i;
    for (i = 1; i < argc; i++)
        if (same_command(argv[i], "/TRACE") || same_command(argv[i], "-TRACE")) return 1;
    return 0;
}

static int wants_write_confirm(argc, argv)
int argc;
char **argv;
{
    int i;
    for (i = 1; i < argc; i++)
        if (same_command(argv[i], "/WRITE") ||
            same_command(argv[i], "-WRITE")) return 1;
    return 0;
}

static int write_gate_valid()
{
    FILE *file;
    char line[32];
    int valid;

    file = fopen(WRITE_GATE_NAME, "rb");
    if (!file) return 0;
    valid = fgets(line, sizeof(line), file) != 0 &&
            !strncmp(line, "WILLEM-WRITE-GATE-1", 19) &&
            (line[19] == '\r' || line[19] == '\n' || line[19] == 0);
    fclose(file);
    return valid;
}

static void trace_begin(ctx)
struct dos_context *ctx;
{
    unsigned d[7];
    unsigned char header[20];
    int i;

    if (!ctx->trace) return;
    memset(header, 0, sizeof(header));
    memcpy(header, "WLT1", 4);
    dos_datetime(d);
    for (i = 0; i < 7; i++) {
        header[4 + i * 2] = (unsigned char)(d[i] & 0xffU);
        header[5 + i * 2] = (unsigned char)(d[i] >> 8);
    }
    header[18] = (unsigned char)(ctx->base & 0xffU);
    header[19] = (unsigned char)(ctx->base >> 8);
    fwrite(header, 1, sizeof(header), ctx->trace);
    fflush(ctx->trace);
}

static void trace_end(ctx)
struct dos_context *ctx;
{
    unsigned char footer[12];
    unsigned long count;
    if (!ctx->trace) return;
    memset(footer, 0, sizeof(footer));
    memcpy(footer, "WLE1", 4);
    count = ctx->sequence;
    footer[4] = (unsigned char)(count & 0xffUL);
    footer[5] = (unsigned char)((count >> 8) & 0xffUL);
    footer[6] = (unsigned char)((count >> 16) & 0xffUL);
    footer[7] = (unsigned char)((count >> 24) & 0xffUL);
    fwrite(footer, 1, sizeof(footer), ctx->trace);
    fflush(ctx->trace);
}

static void usage()
{
    puts("WILLEM V30 read/check/gated-write utility");
    puts("Usage: WILLEM RRF5  output.bin [base] [/PROFILE:name] [/TRACE]");
    puts("       WILLEM R2764 output.bin [base] [/PROFILE:name] [/TRACE]");
    puts("       WILLEM R28C64 output.bin [base] [/PROFILE:name] [/TRACE]");
    puts("       WILLEM B2764 [base] [/TRACE]");
    puts("       WILLEM B28C64 [base] [/TRACE]");
    puts("       WILLEM V2764 image.bin [base] [/TRACE]");
    puts("       WILLEM V28C64 image.bin [base] [/TRACE]");
    puts("       WILLEM W28C64 image.bin [base] /WRITE [/TRACE]");
    puts("       WILLEM D28C64 [base] [/TRACE]  (chip removed)");
}

static int decode_command(text, action, device)
char *text;
int *action;
int *device;
{
    if (same_command(text, "RRF5") || same_command(text, "R2716")) {
        *action = ACTION_READ; *device = DEVICE_RF5;
    } else if (same_command(text, "R2764")) {
        *action = ACTION_READ; *device = DEVICE_2764;
    } else if (same_command(text, "R28C64")) {
        *action = ACTION_READ; *device = DEVICE_28C64;
    } else if (same_command(text, "B2764")) {
        *action = ACTION_BLANK; *device = DEVICE_2764;
    } else if (same_command(text, "B28C64")) {
        *action = ACTION_BLANK; *device = DEVICE_28C64;
    } else if (same_command(text, "V2764")) {
        *action = ACTION_VERIFY; *device = DEVICE_2764;
    } else if (same_command(text, "V28C64")) {
        *action = ACTION_VERIFY; *device = DEVICE_28C64;
    } else if (same_command(text, "W28C64")) {
        *action = ACTION_WRITE; *device = DEVICE_28C64;
    } else if (same_command(text, "D28C64")) {
        *action = ACTION_DIAG; *device = DEVICE_28C64;
    } else {
        return 0;
    }
    return 1;
}

static char *action_name(action)
int action;
{
    if (action == ACTION_READ) return "READ";
    if (action == ACTION_BLANK) return "BLANK";
    if (action == ACTION_WRITE) return "WRITE";
    if (action == ACTION_DIAG) return "DIAGNOSTIC";
    return "VERIFY";
}

static char *device_name(device)
int device;
{
    if (device == DEVICE_RF5) return "K573RF5/2716";
    return device == DEVICE_2764 ? "2764/27C64" : "AT28C64";
}

static unsigned device_size(device)
int device;
{
    return device == DEVICE_RF5 ? RF5_SIZE : ROM_SIZE;
}

static unsigned device_dip_mask(device)
int device;
{
    return device == DEVICE_RF5 ? 0x1a3U : 0x12bU;
}

static void display_dips(mask)
unsigned mask;
{
    char on_row[40];
    char off_row[40];
    int i, pos;

    pos = 0;
    for (i = 0; i < 12; i++) {
        on_row[pos] = '[';
        on_row[pos + 1] = (mask & (1U << i)) ? 'X' : ' ';
        on_row[pos + 2] = ']';
        off_row[pos] = '[';
        off_row[pos + 1] = (mask & (1U << i)) ? ' ' : 'X';
        off_row[pos + 2] = ']';
        pos += 3;
    }
    on_row[pos] = 0;
    off_row[pos] = 0;
    logmsg("DIP number:  1  2  3  4  5  6  7  8  9 10 11 12");
    logmsg("DIP ON :   %s", on_row);
    logmsg("DIP OFF:   %s", off_row);
    logmsg("Set each numbered lever toward the row containing X");
}

static void display_zif(device)
int device;
{
    if (device == DEVICE_RF5) {
        logmsg("ZIF pair: lever/notch -> [--][--][--][--][1/24] ... [12/13]");
        logmsg("Leave FOUR rows empty at lever end; place chip at far end");
        logmsg("Use SPECIAL 2716 route; notch faces empty rows and lever");
    } else {
        logmsg("ZIF pair: lever/notch -> [--][--][1/28] ... [14/15] <- far end");
        logmsg("Leave TWO complete rows empty at lever end; do NOT center chip");
        logmsg("Place chip against far end; notch faces empty rows and lever");
    }
}

static void wait_enter(message)
char *message;
{
    int ch;
    logmsg("PAUSE: %s", message);
    logmsg("Press ENTER to continue");
    fflush(stdout);
    do {
        ch = getchar();
    } while (ch != '\n' && ch != '\r' && ch != EOF);
}

static void log_ports(context, label)
struct dos_context *context;
char *label;
{
    unsigned base;
    base = context->base;
    logmsg("%s: DATA=%02X STATUS=%02X CONTROL=%02X",
           label, dos_inb(base), dos_inb(base + 1), dos_inb(base + 2));
}

static void run_28c64_diagnostic(wl, context)
struct willem *wl;
struct dos_context *context;
{
    wl_u8 empty_read;

    logmsg("DIAGNOSTIC IS NON-WRITING AND MUST BE RUN WITH SOCKET EMPTY");
    logmsg("Use DIP 12Bh for this routing test: ON 1,2,4,6,9");
    wait_enter("Confirm chip REMOVED; meter ground: AT pin 14 = ZIF contact 16");

    wl_vpp(wl, 0);
    wl_oe(wl, 1);
    wl_we(wl, 1);
    wl_vcc(wl, 1);
    port_delay(context, 50000);
    port_delay(context, 50000);
    port_delay(context, 50000);
    port_delay(context, 50000);
    log_ports(context, "VCC on; OE inactive; WE inactive");
    logmsg("Measure AT pins (ZIF contacts): VCC 28(30), WE 27(29), OE 22(24), CE 20(22)");
    wait_enter("Expect VCC about 5V, /WE HIGH, /OE HIGH; record /CE");

    wl_set_address(wl, 0x0000UL, WL_ADDR_FIRST_BIT);
    wl_set_data(wl, 0x55);
    log_ports(context, "Address 0000h; data 55h; controls inactive");
    wl_set_address(wl, 0x1fffUL, WL_ADDR_FIRST_BIT);
    wl_set_data(wl, 0xaa);
    log_ports(context, "Address 1FFFh; data AAh; controls inactive");

    wl_set_address(wl, 0x0000UL, WL_ADDR_FIRST_BIT);
    wl_set_data(wl, 0x3e);
    wl_we(wl, 0);
    log_ports(context, "WE asserted and held; OE inactive; data 3Eh");
    wait_enter("Measure /WE AT27(ZIF29) LOW; record /CE AT20(ZIF22); CHIP OUT");
    wl_we(wl, 1);
    log_ports(context, "WE returned inactive");

    wl_oe(wl, 0);
    log_ports(context, "OE asserted and held; WE inactive");
    wait_enter("Measure /OE AT22(ZIF24) LOW; record /CE AT20(ZIF22); CHIP OUT");
    empty_read = wl_get_data(wl);
    logmsg("Empty-socket serial read sample=%02X (normally FF; informational only)",
           empty_read);
    wl_oe(wl, 1);
    wl_we(wl, 1);
    log_ports(context, "Controls returned inactive before shutdown");
}

int main(argc, argv)
int argc;
char **argv;
{
    struct dos_context context;
    struct wl_io io;
    struct willem wl;
    FILE *file;
    char *image_name;
    unsigned address, base, crc, mismatch_count, written, unchanged;
    unsigned retry_bytes, total_retries, late_bytes;
    unsigned power_on_ms, image_size, dip_mask;
    unsigned long read_started, read_ms, program_started, program_ms;
    unsigned long verify_started, verify_ms, image_crc32;
    unsigned char actual, expected;
    int result, action, device, first_option, i, base_seen, powered, profile_seen;
    int write_failed, attempt;
    char *profile_name;
    struct read_profile *profile;

    result = 1;
    powered = 0;
    file = 0;
    image_name = 0;
    context.trace = 0;
    context.sequence = 0;
    profile_name = "legacy";
    profile_seen = 0;
    profile = 0;
    read_started = 0;
    read_ms = 0;
    program_started = 0;
    program_ms = 0;
    verify_started = 0;
    verify_ms = 0;
    image_crc32 = 0;
    log_file = open_append(LOG_NAME, 0);
    logmsg("================ BEGIN RUN ================");
    logmsg("Willem V30 version %s; 8086-compatible; 24-bit address build",
           WILLEM_VERSION);
    logmsg("Command-line argc=%d", argc);
    for (address = 0; address < (unsigned)argc; address++)
        logmsg("argv[%u]=<%s>", address, argv[address]);

    if (argc < 2 || !decode_command(argv[1], &action, &device)) {
        usage();
        logmsg("ERROR: invalid command line");
        goto done;
    }
    image_size = device_size(device);
    dip_mask = device_dip_mask(device);

    if (action == ACTION_READ || action == ACTION_VERIFY ||
        action == ACTION_WRITE) {
        if (argc < 3) {
            usage();
            logmsg("ERROR: command requires an image filename");
            goto done;
        }
        image_name = argv[2];
        first_option = 3;
    } else {
        first_option = 2;
    }

    base = 0x378;
    base_seen = 0;
    for (i = first_option; i < argc; i++) {
        char *selected;
        if (same_command(argv[i], "/TRACE") ||
            same_command(argv[i], "-TRACE")) continue;
        if (action == ACTION_WRITE &&
            (same_command(argv[i], "/WRITE") ||
             same_command(argv[i], "-WRITE"))) continue;
        selected = profile_value(argv[i]);
        if (selected) {
            if (action != ACTION_READ || !*selected || profile_seen) {
                usage();
                logmsg("ERROR: /PROFILE is read-only and may appear once");
                goto done;
            }
            profile_name = selected;
            profile_seen = 1;
            continue;
        }
        if (base_seen || !(base = parse_base(argv[i]))) {
            usage();
            logmsg("ERROR: invalid or duplicate option <%s>", argv[i]);
            goto done;
        }
        base_seen = 1;
    }
    profile = find_profile(profile_name);
    if (action == ACTION_READ && !profile) {
        usage();
        logmsg("ERROR: unknown read profile <%s>", profile_name);
        goto done;
    }

    context.base = base;
    if (action == ACTION_WRITE) {
        if (!wants_write_confirm(argc, argv)) {
            logmsg("ERROR: W28C64 requires explicit /WRITE confirmation");
            goto done;
        }
        if (!write_gate_valid()) {
            logmsg("ERROR: valid %s is required; physical read gate is locked",
                   WRITE_GATE_NAME);
            goto done;
        }
        logmsg("Write gate accepted from %s", WRITE_GATE_NAME);
        if (wants_trace(argc, argv)) {
            logmsg("ERROR: /TRACE is forbidden during SDP writes; it can violate tBLC=150us");
            goto done;
        }
    }
    if (wants_trace(argc, argv)) {
        context.trace = open_append(TRACE_NAME, 1);
        if (!context.trace) {
            logmsg("ERROR: cannot append %s", TRACE_NAME);
            goto done;
        }
        trace_begin(&context);
    }

    if (action == ACTION_READ) {
        file = fopen(image_name, "wb");
        if (!file) {
            logmsg("ERROR: cannot create output file %s", image_name);
            goto close_trace;
        }
    } else if (action == ACTION_VERIFY || action == ACTION_WRITE) {
        file = fopen(image_name, "rb");
        if (!file) {
            logmsg("ERROR: cannot open reference image %s", image_name);
            goto close_trace;
        }
        if (fread(rom_buffer, 1, image_size, file) != image_size ||
            fgetc(file) != EOF) {
            logmsg("ERROR: reference image must be exactly %u bytes", image_size);
            fclose(file);
            file = 0;
            goto close_trace;
        }
        fclose(file);
        file = 0;
    }

    io.ctx = &context;
    io.data_write = port_data_write;
    io.control_write = port_control_write;
    io.status_read = port_status_read;
    io.delay_us = port_delay;
    wl_init(&wl, &io);

    power_on_ms = device == DEVICE_28C64 ? 200U : 5U;
    if (action == ACTION_READ) {
        power_on_ms = device == DEVICE_28C64 ? profile->power_28c64_ms
                                             : profile->power_2764_ms;
        wl_set_read_timing(&wl, profile->address_setup_us,
                           profile->oe_settle_us,
                           profile->input_latch_us,
                           profile->input_clock_us, power_on_ms);
        logmsg("DOSRAVI_PROFILE name=%s address_setup_us=%u oe_settle_us=%u input_latch_us=%u input_clock_us=%u power_on_ms=%u build_id=%s",
               profile->name, profile->address_setup_us,
               profile->oe_settle_us, profile->input_latch_us,
               profile->input_clock_us, power_on_ms, WILLEM_BUILD_ID);
    }

    logmsg("Operation=%s device=%s image=%s LPT=%03Xh trace=%s",
           action_name(action), device_name(device),
           image_name ? image_name : "(none)", base,
           context.trace ? "on" : "off");
    logmsg("Required DIP mask=%03Xh; VPP MUST remain off", dip_mask);
    display_dips(dip_mask);
    display_zif(device);
    logmsg("Initial raw DATA=%02X STATUS=%02X CONTROL=%02X",
           dos_inb(base), dos_inb(base + 1), dos_inb(base + 2));
    if (action == ACTION_DIAG) {
        run_28c64_diagnostic(&wl, &context);
        powered = 1;
        logmsg("Diagnostic complete; power transition: safe shutdown begins");
        wl_end_read(&wl);
        powered = 0;
        logmsg("Safe shutdown complete: VCC off, VPP off");
        result = 0;
        goto close_trace;
    }

    logmsg("Power transition: enabling VCC, VPP off");
    if (action == ACTION_READ) read_started = dos_bios_ticks();
    if (action == ACTION_WRITE) wl_begin_28c64_write(&wl);
    else if (device == DEVICE_RF5) wl_begin_2716_read(&wl);
    else if (device == DEVICE_2764) wl_begin_2764_read(&wl);
    else wl_begin_28c64_read(&wl);
    powered = 1;

    mismatch_count = 0;
    written = 0;
    unchanged = 0;
    write_failed = 0;
    retry_bytes = 0;
    total_retries = 0;
    late_bytes = 0;
    if (action == ACTION_WRITE) {
        crc = crc16(rom_buffer, image_size);
        image_crc32 = crc32(rom_buffer, image_size);
        logmsg("SDP protected programming begins: bytes=%u image CRC16-CCITT=%04X",
               image_size, crc);
        program_started = dos_bios_ticks();
        for (address = 0; address < image_size; address++) {
            actual = wl_read_byte(&wl, address);
            if (actual == rom_buffer[address]) {
                unchanged++;
            } else {
                for (attempt = 1; attempt <= 3; attempt++) {
                    /* Direct assembly masks interrupts and keeps the four SDP
                       loads within tBLC. Datasheet maximum tWC is 10 ms. */
                    dos_sdp_write(base, address, rom_buffer[address]);
                    dos_wait_us(12000U);
                    actual = wl_read_byte(&wl, address);
                    if (actual == rom_buffer[address]) break;

                    /* One late check distinguishes a slow/marginal cell from
                       a rejected SDP sequence without adding another cycle. */
                    dos_wait_us(10000U);
                    actual = wl_read_byte(&wl, address);
                    if (actual == rom_buffer[address]) {
                        late_bytes++;
                        logmsg("Late completion at %04Xh on attempt %d",
                               address, attempt);
                        break;
                    }
                    if (attempt < 3) {
                        total_retries++;
                        logmsg("RETRY at %04Xh: attempt=%d read=%02X wanted=%02X",
                               address, attempt, actual, rom_buffer[address]);
                    }
                }
                if (actual != rom_buffer[address]) {
                    logmsg("ERROR: SDP write failed after 3 attempts at %04Xh: read=%02X wanted=%02X",
                           address, actual, rom_buffer[address]);
                    write_failed = 1;
                    break;
                } else {
                    written++;
                    if (attempt > 1) retry_bytes++;
                }
            }
            if ((address & 0x00ffU) == 0x00ffU)
                logmsg("Write progress: %u/%u bytes, written=%u unchanged=%u",
                       address + 1, image_size, written, unchanged);
        }
        program_ms = (dos_bios_ticks() - program_started) * 55UL;
        if (!write_failed) {
            logmsg("Programming pass complete: written=%u unchanged=%u retry-bytes=%u retries=%u late=%u",
                   written, unchanged, retry_bytes, total_retries, late_bytes);
            verify_started = dos_bios_ticks();
            for (address = 0; address < image_size; address++) {
                actual = wl_read_byte(&wl, address);
                expected = rom_buffer[address];
                if (actual != expected) {
                    mismatch_count++;
                    if (mismatch_count <= 8)
                        logmsg("Post-write mismatch at %04Xh: read=%02X expected=%02X",
                               address, actual, expected);
                }
                if ((address & 0x01ffU) == 0x01ffU)
                    logmsg("Post-write verify: %u/%u bytes", address + 1,
                           image_size);
            }
            verify_ms = (dos_bios_ticks() - verify_started) * 55UL;
        }
    } else {
        for (address = 0; address < image_size; address++) {
            actual = wl_read_byte(&wl, address);
            if (action == ACTION_READ) {
                rom_buffer[address] = actual;
            } else {
                expected = action == ACTION_BLANK ? 0xffU : rom_buffer[address];
                if (actual != expected) {
                    mismatch_count++;
                    if (mismatch_count <= 8)
                        logmsg("Mismatch at %04Xh: read=%02X expected=%02X",
                               address, actual, expected);
                }
            }
            if ((address & 0x01ffU) == 0x01ffU)
                logmsg("Scan progress: %u/%u bytes", address + 1, image_size);
        }
    }

    if (action == ACTION_READ) {
        read_ms = (dos_bios_ticks() - read_started) * 55UL;
        logmsg("DOSRAVI_METRIC read_ms=%lu profile=%s", read_ms,
               profile->name);
    } else if (action == ACTION_WRITE) {
        logmsg("DOSRAVI_WRITE_METRIC program_ms=%lu verify_ms=%lu changed=%u unchanged=%u retry_bytes=%u retries=%u late=%u image_crc32=%08lX build_id=%s",
               program_ms, verify_ms, written, unchanged, retry_bytes,
               total_retries, late_bytes, image_crc32, WILLEM_BUILD_ID);
    }

    logmsg("Power transition: safe shutdown begins");
    wl_end_read(&wl);
    powered = 0;
    logmsg("Safe shutdown complete: VCC off, VPP off");

    if (action == ACTION_WRITE) {
        if (write_failed) {
            logmsg("WRITE FAILED during programming; post-write verify skipped");
            result = 1;
        } else if (mismatch_count) {
            logmsg("WRITE FAILED: post-write mismatches=%u (first 8 shown)",
                   mismatch_count);
            result = 2;
        } else {
            logmsg("WRITE PASSED: programmed=%u unchanged=%u verified=%u retry-bytes=%u retries=%u late=%u",
                   written, unchanged, image_size, retry_bytes, total_retries,
                   late_bytes);
            result = 0;
        }
    } else if (action == ACTION_READ) {
        if (fwrite(rom_buffer, 1, image_size, file) != image_size) {
            logmsg("ERROR: failed writing output image");
            fclose(file);
            file = 0;
            goto close_trace;
        }
        fclose(file);
        file = 0;
        crc = crc16(rom_buffer, image_size);
        logmsg("Read complete: bytes=%u CRC16-CCITT=%04X", image_size, crc);
        result = 0;
    } else if (mismatch_count) {
        logmsg("%s FAILED: mismatches=%u (first 8 shown)",
               action_name(action), mismatch_count);
        result = 2;
    } else {
        logmsg("%s PASSED: all %u bytes match %s", action_name(action),
               image_size, action == ACTION_BLANK ? "FFh" : image_name);
        result = 0;
    }

close_trace:
    if (powered) {
        logmsg("Power transition: emergency safe shutdown begins");
        wl_end_read(&wl);
        logmsg("Emergency safe shutdown complete: VCC off, VPP off");
    }
    if (file) fclose(file);
    if (context.trace) {
        trace_end(&context);
        fclose(context.trace);
    }
done:
    logmsg("Result code=%d", result);
    logmsg("================= END RUN =================");
    if (log_file) fclose(log_file);
    return result;
}
