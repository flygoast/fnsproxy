#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include "errdsp.h"

#define BUF_SIZE        4096

#define ED_RESET        0
#define ED_BRIGHT       1
#define ED_DIM          2
#define ED_UNDERLINE    3
#define ED_BLINK        4
#define ED_REVERSE      7
#define ED_HIDDEN       8

static void __v_message(int level, const char *fmt, va_list ap) {
    char    buf[BUF_SIZE];
    char    msg[BUF_SIZE + 1024];
    char    pmode[64];
    char    lmode[64];

    vsnprintf(buf, sizeof(buf), fmt, ap);
    if (errno == 0 || errno == ENOSYS || level == ED_DEBUG) {
        snprintf(msg, sizeof(msg), "%s\n", buf);
    } else {
        snprintf(msg, sizeof(msg), "%s: %s\n", buf, strerror(errno));
    }

    switch (level) {
    case ED_DEBUG:
        snprintf(pmode, sizeof(pmode), "[%c[%d;%dmdebug%c[%dm]", 0x1B,
            ED_BRIGHT, ED_BLUE + 30, 0x1B, ED_RESET);
        strcpy(lmode, "[debug]");
        break;
    case ED_ALERT:
        snprintf(pmode, sizeof(pmode), "[%c[%d;%dmalert%c[%dm]", 0x1B,
            ED_BRIGHT, ED_GREEN + 30, 0x1B, ED_RESET);
        strcpy(lmode, "[alert]");
        break;
    case ED_ERROR:
        snprintf(pmode, sizeof(pmode), "[%c[%d;%dmerror%c[%dm]", 0x1B,
            ED_BRIGHT, ED_YELLOW + 30, 0x1B, ED_RESET);
        strcpy(lmode, "[error]");
        break;
    case ED_FATAL:
        snprintf(pmode, sizeof(pmode), "[%c[%d;%dmfatal%c[%dm]", 0x1B,
            ED_BRIGHT, ED_RED + 30, 0x1B, ED_RESET);
        strcpy(lmode, "[fatal]");
        break;
    }

    if (isatty(STDERR_FILENO)) {
        fprintf(stderr, "%s %s", pmode, msg);
    } else {
        fprintf(stderr, "%s %s", lmode, msg);
    }
}

void err_notify(int level, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    __v_message(level, fmt, ap);
    va_end(ap);
}

static void __v_display(int color, const char *fmt, va_list ap) {
    char buf[BUF_SIZE];
    char msg[BUF_SIZE + 1024];

    vsnprintf(buf, sizeof(buf), fmt, ap);
    if (isatty(STDOUT_FILENO)) {
        snprintf(msg, sizeof(msg), "%c[%d;%dm%s%c[%dm\n", 0x1B, ED_RESET, 
            color + 30, buf, 0x1B, ED_RESET);
        printf("%s", msg);
    } else {
        printf("%s", buf);
    }
}

void msg_display(int color, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    __v_display(color, fmt, ap);
    va_end(ap);
}

/* gcc errdsp.c -D ERRDSP_TEST_MAIN */
#ifdef ERRDSP_TEST_MAIN
int main(int argc, char **argv) {
    errno = 1;
    err_notify(ED_DEBUG, "%s", "Test a debug notify");
    err_notify(ED_ALERT, "%s", "Test a alert notify");
    err_notify(ED_ERROR, "%s", "Test a error notify");
    err_notify(ED_FATAL, "%s", "Test a fatal notify");

    msg_display(ED_CYAN, "%s", "Test a msg display of cyan color");
    msg_display(ED_BLACK, "%s", "Test a msg display of black color");
    msg_display(ED_MAGENTA, "%s", "Test a msg display of magenta color");
    exit(0);
}
#endif /* ERRDSP_TEST_MAIN */
