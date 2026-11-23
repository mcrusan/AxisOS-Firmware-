// -----------------------------------------------------------------------------
// File:        firmware/src/axisOS_core.c
// Description: AxisOS core subsystem implementation (hardware watchdog supervision, tick counter, freestanding formatted logging — zero <stdio.h>/<stdarg.h>).
// Author:      Marc F. Crusan (MFC) / Crusan & Co. w/ PP & AGY (AXEL)
// Date:        2026112302
// Version:     v1.8.3-Silicon
// -----------------------------------------------------------------------------
// 2026112302 (MFC/AGY) Eliminated <stdio.h> and <stdarg.h>; implemented freestanding core_puts/core_printf/core_log over putchar_raw() + compiler __builtin_va_*.
// 2026112301 (MFC/AGY) Removed unused <string.h> include per AxisOS Vanilla header audit.
// 2026110901 (MFC/PP/AGY) Upgraded to canonical Crusan & Co. 5-Field SOP Header standard.

#include "pico/stdlib.h"
#include "pico/time.h"
#include "axis-string.h"
#include "axisOS_core.h"
#include "axisOS_version.h"

/* -----------------------------------------------------------------------
 *  Private state
 * --------------------------------------------------------------------- */
static bool               s_ready     = false;
static axisOS_log_level_t s_log_level = LOG_LEVEL_INFO;

static const char *const s_level_str[] = {
    "DBG", "INF", "WRN", "ERR"
};

/* -----------------------------------------------------------------------
 *  Freestanding Console Formatter (zero <stdio.h>, zero <stdarg.h>)
 * --------------------------------------------------------------------- */
static void core_putc(char c) {
    putchar_raw((int)(unsigned char)c);
}

void core_puts(const char *s) {
    if (!s) s = "(null)";
    while (*s) core_putc(*s++);
}

static void core_put_pad(char ch, int count) {
    while (count-- > 0) core_putc(ch);
}

static void core_put_str_fmt(const char *s, int width, bool left_align) {
    if (!s) s = "(null)";
    int len = (int)axis_strlen(s);
    int pad = (width > len) ? (width - len) : 0;
    if (!left_align) core_put_pad(' ', pad);
    core_puts(s);
    if (left_align)  core_put_pad(' ', pad);
}

static void core_put_uint_fmt(unsigned long v, int base, bool upper,
                              int width, bool zero_pad, bool left_align) {
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[32];
    int n = 0;
    if (v == 0) {
        tmp[n++] = '0';
    } else {
        while (v > 0 && n < 31) {
            tmp[n++] = digits[v % (unsigned long)base];
            v /= (unsigned long)base;
        }
    }
    int pad = (width > n) ? (width - n) : 0;
    if (!left_align) core_put_pad(zero_pad ? '0' : ' ', pad);
    while (n > 0) core_putc(tmp[--n]);
    if (left_align)  core_put_pad(' ', pad);
}

static void core_put_int_fmt(long v, int width, bool zero_pad, bool left_align) {
    unsigned long uv;
    bool neg = false;
    if (v < 0) {
        neg = true;
        uv = (unsigned long)(-(v + 1)) + 1ul;
    } else {
        uv = (unsigned long)v;
    }
    char tmp[32];
    int n = 0;
    if (uv == 0) {
        tmp[n++] = '0';
    } else {
        while (uv > 0 && n < 31) {
            tmp[n++] = (char)('0' + (uv % 10ul));
            uv /= 10ul;
        }
    }
    int total = n + (neg ? 1 : 0);
    int pad = (width > total) ? (width - total) : 0;
    if (!left_align && !zero_pad) core_put_pad(' ', pad);
    if (neg) core_putc('-');
    if (!left_align && zero_pad)  core_put_pad('0', pad);
    while (n > 0) core_putc(tmp[--n]);
    if (left_align) core_put_pad(' ', pad);
}

static void core_put_float_fmt(double v, int prec) {
    if (prec < 0) prec = 2;
    if (prec > 6) prec = 6;
    if (v < 0.0) {
        core_putc('-');
        v = -v;
    }
    unsigned long scale = 1ul;
    for (int i = 0; i < prec; i++) scale *= 10ul;
    unsigned long scaled = (unsigned long)(v * (double)scale + 0.5);
    unsigned long ipart  = scaled / scale;
    unsigned long fpart  = scaled % scale;
    core_put_uint_fmt(ipart, 10, false, 0, false, false);
    if (prec > 0) {
        core_putc('.');
        core_put_uint_fmt(fpart, 10, false, prec, true, false);
    }
}

static void core_vprintf(const char *fmt, __builtin_va_list ap) {
    if (!fmt) return;
    while (*fmt) {
        if (*fmt != '%') {
            core_putc(*fmt++);
            continue;
        }
        fmt++;
        if (*fmt == '%') { core_putc('%'); fmt++; continue; }

        bool left_align = false;
        bool zero_pad   = false;
        int  width      = 0;
        int  prec       = -1;
        bool is_long    = false;

        while (*fmt == '-' || *fmt == '0') {
            if (*fmt == '-') left_align = true;
            if (*fmt == '0') zero_pad = true;
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        if (*fmt == '.') {
            fmt++;
            prec = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                prec = prec * 10 + (*fmt - '0');
                fmt++;
            }
        }
        if (*fmt == 'l') { is_long = true; fmt++; }

        switch (*fmt) {
            case 's':
                core_put_str_fmt(__builtin_va_arg(ap, const char *), width, left_align);
                break;
            case 'c':
                core_putc((char)__builtin_va_arg(ap, int));
                break;
            case 'd':
            case 'i': {
                long val = is_long ? __builtin_va_arg(ap, long) : (long)__builtin_va_arg(ap, int);
                core_put_int_fmt(val, width, zero_pad, left_align);
                break;
            }
            case 'u': {
                unsigned long val = is_long ? __builtin_va_arg(ap, unsigned long) : (unsigned long)__builtin_va_arg(ap, unsigned int);
                core_put_uint_fmt(val, 10, false, width, zero_pad, left_align);
                break;
            }
            case 'x':
            case 'X': {
                unsigned long val = is_long ? __builtin_va_arg(ap, unsigned long) : (unsigned long)__builtin_va_arg(ap, unsigned int);
                core_put_uint_fmt(val, 16, (*fmt == 'X'), width, zero_pad, left_align);
                break;
            }
            case 'f':
                core_put_float_fmt(__builtin_va_arg(ap, double), prec);
                break;
            default:
                core_putc('%');
                if (*fmt) core_putc(*fmt);
                break;
        }
        if (*fmt) fmt++;
    }
}

void core_printf(const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    core_vprintf(fmt, args);
    __builtin_va_end(args);
}

/* -----------------------------------------------------------------------
 *  Public: tick
 * --------------------------------------------------------------------- */
uint32_t core_tick_ms(void) {
    return (uint32_t)(to_ms_since_boot(get_absolute_time()));
}

/* -----------------------------------------------------------------------
 *  Public: logging
 * --------------------------------------------------------------------- */
void core_log_set_level(axisOS_log_level_t level) {
    if (level <= LOG_LEVEL_ERROR) {
        s_log_level = level;
    }
}

void core_log(axisOS_log_level_t level, const char *tag, const char *fmt, ...) {
    if (level < s_log_level || level > LOG_LEVEL_ERROR) return;

    core_printf("[%s][%7lu][%s] ", s_level_str[level], (unsigned long)core_tick_ms(), tag ? tag : "SYS");

    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    core_vprintf(fmt, args);
    __builtin_va_end(args);

    core_puts("\r\n");
}

/* -----------------------------------------------------------------------
 *  Public: lifecycle
 * --------------------------------------------------------------------- */
axisOS_status_t core_init(void) {
    if (s_ready) return AXISOS_OK;

    LOG_I("CORE", "Crusan & Co. // AxisOS %s initialising...", AXISOS_VERSION_STRING);

    s_ready = true;
    LOG_I("CORE", "Core subsystem ready.");
    return AXISOS_OK;
}

void core_deinit(void) {
    s_ready = false;
}

bool core_is_ready(void) {
    return s_ready;
}
