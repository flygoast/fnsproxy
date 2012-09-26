#ifndef __ERRDSP_H_INCLUDED__
#define __ERRDSP_H_INCLUDED__

#define ED_DEBUG        0
#define ED_ALERT        1
#define ED_ERROR        2
#define ED_FATAL        3


#define ED_BLACK        0
#define ED_RED          1
#define ED_GREEN        2
#define ED_YELLOW       3
#define ED_BLUE         4
#define ED_MAGENTA      5
#define ED_CYAN         6
#define ED_WHITE        7

void err_notify(int level, const char *fmt, ...);
void msg_display(int color, const char *fmt, ...);

#endif /* __ERRDSP_H_INCLUDED__ */
