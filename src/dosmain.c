#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "willem.h"

#define ROM_SIZE 8192
#define LOG_NAME "WILLEM.LOG"
#define TRACE_NAME "WTRACE.BIN"

extern void dos_outb(unsigned port, unsigned value);
extern unsigned dos_inb(unsigned port);
extern void dos_wait_us(unsigned usec);
extern void dos_datetime(unsigned *fields);

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

static unsigned parse_base(text)
char *text;
{
    unsigned value;
    if (!text) return 0x378;
    if (sscanf(text, "%x", &value) != 1) return 0;
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

static int wants_trace(argc, argv)
int argc;
char **argv;
{
    int i;
    for (i = 1; i < argc; i++)
        if (same_command(argv[i], "/TRACE") || same_command(argv[i], "-TRACE")) return 1;
    return 0;
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
    puts("WILLEM V30 read-only prototype");
    puts("Usage: WILLEM R2764 output.bin [base] [/TRACE]");
    puts("Example: WILLEM R2764 JU1764.BIN 378 /TRACE");
}

int main(argc, argv)
int argc;
char **argv;
{
    struct dos_context context;
    struct wl_io io;
    struct willem wl;
    FILE *output;
    unsigned address, base, crc;
    int result;

    result = 1;
    log_file = open_append(LOG_NAME, 0);
    logmsg("================ BEGIN RUN ================");
    logmsg("Willem V30 read-only prototype; 8086 build");
    logmsg("Command-line argc=%d", argc);
    for (address = 0; address < (unsigned)argc; address++)
        logmsg("argv[%u]=<%s>", address, argv[address]);

    if (argc < 3 || !same_command(argv[1], "R2764")) {
        usage();
        logmsg("ERROR: invalid command line");
        goto done;
    }

    base = parse_base(argc > 3 ? argv[3] : 0);
    if (!base) {
        logmsg("ERROR: invalid LPT base address");
        goto done;
    }

    context.base = base;
    context.sequence = 0;
    context.trace = 0;
    if (wants_trace(argc, argv)) {
        context.trace = open_append(TRACE_NAME, 1);
        if (!context.trace) {
            logmsg("ERROR: cannot append %s", TRACE_NAME);
            goto done;
        }
        trace_begin(&context);
    }

    output = fopen(argv[2], "wb");
    if (!output) {
        logmsg("ERROR: cannot create output file %s", argv[2]);
        goto close_trace;
    }

    io.ctx = &context;
    io.data_write = port_data_write;
    io.control_write = port_control_write;
    io.status_read = port_status_read;
    io.delay_us = port_delay;
    wl_init(&wl, &io);

    logmsg("Operation=R2764 output=%s LPT=%03Xh trace=%s",
           argv[2], base, context.trace ? "on" : "off");
    logmsg("Required Geepro DIP mask=12Bh; VPP MUST remain off");
    logmsg("Initial raw DATA=%02X STATUS=%02X CONTROL=%02X",
           dos_inb(base), dos_inb(base + 1), dos_inb(base + 2));
    logmsg("Power transition: enabling VCC, VPP off");
    wl_begin_2764_read(&wl);

    for (address = 0; address < ROM_SIZE; address++) {
        rom_buffer[address] = wl_read_byte(&wl, address);
        if ((address & 0x01ffU) == 0x01ffU)
            logmsg("Read progress: %u/%u bytes", address + 1, ROM_SIZE);
    }

    logmsg("Power transition: safe shutdown begins");
    wl_end_read(&wl);
    logmsg("Safe shutdown complete: VCC off, VPP off");

    if (fwrite(rom_buffer, 1, ROM_SIZE, output) != ROM_SIZE) {
        logmsg("ERROR: failed writing output image");
        fclose(output);
        goto close_trace;
    }
    fclose(output);
    crc = crc16(rom_buffer, ROM_SIZE);
    logmsg("Read complete: bytes=%u CRC16-CCITT=%04X", ROM_SIZE, crc);
    result = 0;

close_trace:
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
