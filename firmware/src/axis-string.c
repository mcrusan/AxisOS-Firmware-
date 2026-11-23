// -----------------------------------------------------------------------------
// File:        firmware/src/axis-string.c
// Description: Freestanding zero-libc AxisOS string & memory routines for Firmware
// Author:      Marc F. Crusan (MFC) / Crusan & Co. w/ PP & AGY (AXEL)
// Date:        20260924
// Version:     v0.4-Silicon
// -----------------------------------------------------------------------------
// 2026092400 (MFC) Original AxisOS freestanding axis-string.c (v0.3).
// 2026112301 (MFC/AGY) Adopted into AxisOS-Firmware with pure axis_* namespace (zero Pico SDK ROM symbol collision, zero <string.h>).

#include "axis-string.h"

int axis_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int axis_strncmp(const char *a, const char *b, unsigned long n) {
    while (n && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

unsigned long axis_strlen(const char *s) {
    unsigned long n = 0;
    while (*s++) n++;
    return n;
}

void *axis_memcpy(void *dst, const void *src, unsigned long n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *axis_memset(void *dst, int val, unsigned long n) {
    unsigned char *d = (unsigned char *)dst;
    while (n--) *d++ = (unsigned char)val;
    return dst;
}

char *axis_strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return NULL;
}

char *axis_strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

char *axis_strcat(char *dst, const char *src) {
    char *d = dst;
    while (*d) d++;
    while ((*d++ = *src++));
    return dst;
}

char *axis_strncpy(char *dst, const char *src, unsigned long n) {
    char *d = dst;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dst;
}

char *axis_strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}

int axis_atoi(const char *s) {
    int res = 0;
    int sign = 1;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res * sign;
}

int axis_tolower(int c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

int axis_toupper(int c) {
    if (c >= 'a' && c <= 'z') return c - ('a' - 'A');
    return c;
}

int axis_strcasecmp(const char *a, const char *b) {
    while (*a && (axis_tolower((unsigned char)*a) == axis_tolower((unsigned char)*b))) {
        a++;
        b++;
    }
    return axis_tolower((unsigned char)*a) - axis_tolower((unsigned char)*b);
}

int axis_strncasecmp(const char *a, const char *b, unsigned long n) {
    while (n && *a && (axis_tolower((unsigned char)*a) == axis_tolower((unsigned char)*b))) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return axis_tolower((unsigned char)*a) - axis_tolower((unsigned char)*b);
}

char *axis_strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && (axis_tolower((unsigned char)*h) == axis_tolower((unsigned char)*n))) {
            h++;
            n++;
        }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}
