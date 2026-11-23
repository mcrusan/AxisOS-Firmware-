// -----------------------------------------------------------------------------
// File:        firmware/include/axis-string.h
// Description: Minimal zero-libc string & memory prototypes for AxisOS Firmware
// Author:      Marc F. Crusan (MFC) / Crusan & Co. w/ PP & AGY (AXEL)
// Date:        20260924
// Version:     v0.3-Silicon
// -----------------------------------------------------------------------------
// 2026092400 (MFC) Original AxisOS Tiny freestanding axis-string.h (v0.2).
// 2026112301 (MFC/AGY) Adopted Marc's canonical axis-string.h into AxisOS-Firmware (replacing <string.h>).

#ifndef AXIS_STRING_H
#define AXIS_STRING_H

#ifndef NULL
#define NULL ((void *)0)
#endif

unsigned long axis_strlen(const char *s);
int axis_strcmp(const char *a, const char *b);
int axis_strncmp(const char *a, const char *b, unsigned long n);

char *axis_strcpy(char *dst, const char *src);
char *axis_strcat(char *dst, const char *src);
char *axis_strncpy(char *dst, const char *src, unsigned long n);
char *axis_strchr(const char *s, int c);
char *axis_strstr(const char *haystack, const char *needle);

void *axis_memcpy(void *dst, const void *src, unsigned long n);
void *axis_memset(void *dst, int value, unsigned long n);

int axis_atoi(const char *s);

int axis_tolower(int c);
int axis_toupper(int c);
int axis_strcasecmp(const char *a, const char *b);
int axis_strncasecmp(const char *a, const char *b, unsigned long n);
char *axis_strcasestr(const char *haystack, const char *needle);

#endif /* AXIS_STRING_H */
