/* Copyright (c) 1987, Oliver Laumann, Technical University of Berlin.
 * Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the author is not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 */

enum state_t {
    LIT,         /* Literal input */
    ESC,         /* Start of escape sequence */
    STR,         /* Start of control string */
    TERM,        /* ESC seen in control string */
    CSI,         /* Reading arguments in "CSI Pn ; Pn ; ... ; XXX" */
};

enum string_t {
    NONE,
    DCS,         /* Device control string */
    OSC,         /* Operating system command */
    APC,         /* Application program command */
    PM,          /* Privacy message */
};

#define MAXSTR       128

#define IOSIZE       80

struct win {
    int wpid;
    int ptyfd;
    int aflag;
    char outbuf[IOSIZE];
    int outlen;
    char cmd[MAXSTR];
    char tty[MAXSTR];
    int slot;
    char **image;
    char **attr;
    int active;
    int x, y;
    char LocalAttr;
    int saved;
    int Saved_x, Saved_y;
    char SavedLocalAttr;
    int top, bot;
    int wrap;
    int origin;
    int insert;
    int keypad;
    enum state_t state;
    enum string_t StringType;
    char string[MAXSTR];
    char *stringp;
    char *tabs;
    int vbwait;
    int bell;
};
