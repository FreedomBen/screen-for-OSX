AKAfin	process.c	/^AKAfin(buf, len)$/
ASSERT	screen.h	/^#  define ASSERT(lousy_cpp) {if (!(lousy_cpp)) {de/
ASetMode	ansi.c	/^ASetMode(on)$/
Activate	ansi.c	/^Activate(norefresh)$/
AddCap	termcap.c	/^AddCap(s)$/
AddChar	display.h	/^#define AddChar(c) \\$/
AddLineToHist	ansi.c	/^AddLineToHist(wp, pi, pa, pf)$/
AddLoadav	loadav.c	/^AddLoadav(p)$/
AddStr	display.c	/^AddStr(str)$/
AddStrn	display.c	/^AddStrn(str, n)$/
Attach	attacher.c	/^Attach(how)$/
Attacher	attacher.c	/^Attacher()$/
AttacherChld	attacher.c	/^AttacherChld(SIGDEFARG)$/
AttacherFinit	attacher.c	/^AttacherFinit(SIGDEFARG)$/
AttacherFinitBye	attacher.c	/^AttacherFinitBye(SIGDEFARG)$/
AttacherSigInt	attacher.c	/^AttacherSigInt(SIGDEFARG)$/
AttacherWinch	attacher.c	/^AttacherWinch(SIGDEFARG)$/
BackSpace	ansi.c	/^BackSpace()$/
BackwardTab	ansi.c	/^BackwardTab()$/
BlankResize	display.c	/^BlankResize(wi, he)$/
CPutStr	display.c	/^CPutStr(s, c)$/
CalcCost	display.c	/^CalcCost(s)$/
CatExtra	fileio.c	/^CatExtra(str1, str2)$/
ChangeLine	ansi.c	/^ChangeLine(os, oa, of, y, from, to)$/
ChangeScreenSize	window.c	/^ChangeScreenSize(wi, he, change_fore)$/
ChangeScrollRegion	display.c	/^ChangeScrollRegion(newtop, newbot)$/
ChangeScrollback	window.c	/^ChangeScrollback(p, histheight, histwidth)$/
ChangeWindowSize	window.c	/^ChangeWindowSize(p, wi, he)$/
CheckArgNum	process.c	/^CheckArgNum(nr, args)$/
CheckLP	ansi.c	/^CheckLP(n_ch)$/
CheckMaxSize	window.c	/^CheckMaxSize(wi)$/
CheckPasswd	socket.c	/^CheckPasswd(pwd, pid, utty)$/
CheckPid	socket.c	/^CheckPid(pid)$/
CheckScreenSize	window.c	/^CheckScreenSize(change_flag)$/
Clear	display.c	/^Clear(xs, ys, xe, ye)$/
ClearDisplay	display.c	/^ClearDisplay()$/
ClearFromBOL	ansi.c	/^ClearFromBOL()$/
ClearFromBOS	ansi.c	/^ClearFromBOS()$/
ClearFullLine	ansi.c	/^ClearFullLine()$/
ClearInLine	ansi.c	/^ClearInLine(y, x1, x2)$/
ClearScreen	ansi.c	/^ClearScreen()$/
ClearToEOL	ansi.c	/^ClearToEOL()$/
ClearToEOS	ansi.c	/^ClearToEOS()$/
Colonfin	process.c	/^Colonfin(buf, len)$/
CompileKeys	process.c	/^CompileKeys(s, array)$/
CopyrightAbort	help.c	/^CopyrightAbort()$/
CopyrightProcess	help.c	/^CopyrightProcess(ppbuf, plen)$/
CopyrightRedisplayLine	help.c	/^CopyrightRedisplayLine(y, xs, xe, isblank)$/
CopyrightSetCursor	help.c	/^CopyrightSetCursor()$/
CoreDump	screen.c	/^CoreDump(sig)$/
CountChars	display.c	/^CountChars(c)$/
CountUsers	utmp.c	/^CountUsers()$/
CreateUtmp	utmp.c	/^CreateUtmp(name)$/
Ctrl	screen.h	/^#define Ctrl(c) ((c)&037)$/
CursorDown	ansi.c	/^CursorDown(n)$/
CursorLeft	ansi.c	/^CursorLeft(n)$/
CursorRight	ansi.c	/^CursorRight(n)$/
CursorUp	ansi.c	/^CursorUp(n)$/
CursorkeysMode	display.c	/^CursorkeysMode(on)$/
D2W	window.h	/^#define D2W(y) ((y) + markdata->hist_offset)$/
DISPLAY	display.h	/^# define DISPLAY(x) display->x$/
DebugTTY	tty.c	/^DebugTTY(m)$/
DefClearLine	display.c	/^DefClearLine(y, xs, xe)$/
DefProcess	display.c	/^DefProcess(bufp, lenp)$/
DefRedisplayLine	display.c	/^DefRedisplayLine(y, xs, xe, isblank)$/
DefResize	display.c	/^DefResize(wi, he)$/
DefRestore	display.c	/^DefRestore()$/
DefRewrite	display.c	/^DefRewrite(y, xs, xe, doit)$/
DefSetCursor	display.c	/^DefSetCursor()$/
DeleteChar	ansi.c	/^DeleteChar(n)$/
DeleteLine	ansi.c	/^DeleteLine(n)$/
DesignateCharset	ansi.c	/^DesignateCharset(c, n)$/
Detach	screen.c	/^Detach(mode)$/
DisplayLine	display.c	/^DisplayLine(os, oa, of, s, as, fs, y, from, to)$/
DisplaySleep	screen.c	/^DisplaySleep(d, n)$/
DoAction	process.c	/^DoAction(act, key)$/
DoCSI	ansi.c	/^DoCSI(c, intermediate)$/
DoESC	ansi.c	/^DoESC(c, intermediate)$/
DoLock	attacher.c	/^DoLock(SIGDEFARG)$/
DoResize	window.c	/^DoResize(wi, he)$/
DoScreen	process.c	/^DoScreen(fn, av)$/
DoWait	screen.c	/^DoWait()$/
ExecCreate	socket.c	/^ExecCreate(mp)$/
ExitOverlayPage	display.c	/^ExitOverlayPage()$/
FD_ISSET	os.h	/^# define FD_ISSET(b, fd) ((fd)->fd_bits[0] & 1 << /
FD_SET	os.h	/^# define FD_SET(b, fd) ((fd)->fd_bits[0] |= 1 << (/
FD_ZERO	os.h	/^# define FD_ZERO(fd) ((fd)->fd_bits[0] = 0)$/
Filename	misc.c	/^Filename(s)$/
FillWithEs	ansi.c	/^FillWithEs()$/
FindAKA	ansi.c	/^FindAKA()$/
FindCommnr	process.c	/^FindCommnr(str)$/
FindSocket	socket.c	/^FindSocket(how, fdp)$/
FinishRc	fileio.c	/^FinishRc(rcfilename)$/
Finit	screen.c	/^Finit(i)$/
FinitTerm	display.c	/^FinitTerm()$/
FixLP	display.c	/^FixLP(x2, y2)$/
Flush	display.c	/^Flush()$/
ForwardTab	ansi.c	/^ForwardTab()$/
Free	screen.h	/^#define Free(a) {if ((a) == 0) abort(); else free(/
FreeArray	window.c	/^FreeArray(arr, hi)$/
FreeDisplay	display.c	/^FreeDisplay()$/
FreeKey	process.c	/^FreeKey(key)$/
FreeScrollback	window.c	/^FreeScrollback(p)$/
FreeWindow	process.c	/^FreeWindow(wp)$/
GetHistory	mark.c	/^GetHistory()	\/* return value 1 if copybuffer chang/
GetTTY	tty.c	/^GetTTY(fd, mp)$/
GotoPos	display.c	/^GotoPos(x2, y2)$/
HelpAbort	help.c	/^HelpAbort()$/
HelpProcess	help.c	/^HelpProcess(ppbuf, plen)$/
HelpRedisplayLine	help.c	/^HelpRedisplayLine(y, xs, xe, isblank)$/
HelpSetCursor	help.c	/^HelpSetCursor()$/
INSERTCHAR	display.c	/^INSERTCHAR(c)$/
ISearch	search.c	/^ISearch(dir)$/
InitKeytab	process.c	/^InitKeytab()$/
InitLoadav	loadav.c	/^InitLoadav()$/
InitOverlayPage	display.c	/^InitOverlayPage(datasize, lf, block)$/
InitTTY	tty.c	/^InitTTY(m)$/
InitTerm	display.c	/^InitTerm(adapt)$/
InitTermcap	termcap.c	/^InitTermcap()$/
InitUtmp	utmp.c	/^InitUtmp()$/
InpAbort	input.c	/^InpAbort()$/
InpProcess	input.c	/^InpProcess(ppbuf, plen)$/
InpRedisplayLine	input.c	/^InpRedisplayLine(y, xs, xe, isblank)$/
InpSetCursor	input.c	/^InpSetCursor()$/
Input	input.c	/^Input(istr, len, finfunc, mode)$/
InputAKA	process.c	/^InputAKA()$/
InputColon	process.c	/^InputColon()$/
InsertAChar	ansi.c	/^InsertAChar(c)$/
InsertChar	ansi.c	/^InsertChar(n)$/
InsertLine	ansi.c	/^InsertLine(n)$/
InsertMode	display.c	/^InsertMode(on)$/
IsNum	process.c	/^int IsNum(s, base)$/
IsNumColon	process.c	/^IsNumColon(s, base, p, psize)$/
IsSymbol	screen.c	/^IsSymbol(e, s)$/
KeypadMode	display.c	/^KeypadMode(on)$/
Kill	screen.c	/^Kill(pid, sig)$/
KillBuffers	fileio.c	/^KillBuffers()$/
KillWindow	process.c	/^KillWindow(wi)$/
LineFeed	ansi.c	/^LineFeed(out_mode)$/
LockHup	attacher.c	/^LockHup(SIGDEFARG)$/
LockTerminal	attacher.c	/^LockTerminal()$/
LogToggle	process.c	/^LogToggle(on)$/
MakeBlankLine	ansi.c	/^MakeBlankLine(p, n)$/
MakeClientSocket	socket.c	/^MakeClientSocket(err, name)$/
MakeDisplay	display.c	/^MakeDisplay(uname, utty, term, fd, pid, Mode)$/
MakeNewEnv	screen.c	/^MakeNewEnv()$/
MakeServerSocket	socket.c	/^MakeServerSocket()$/
MakeStatus	display.c	/^MakeStatus(msg)$/
MakeString	termcap.c	/^MakeString(cap, buf, buflen, s)$/
MakeTermcap	termcap.c	/^MakeTermcap(aflag)$/
MakeWinMsg	screen.c	/^MakeWinMsg(s, n)$/
MakeWindow	process.c	/^MakeWindow(newwin)$/
MapCharset	ansi.c	/^MapCharset(n)$/
MarkAbort	mark.c	/^MarkAbort()$/
MarkProcess	mark.c	/^MarkProcess(inbufp,inlenp)$/
MarkRedisplayLine	mark.c	/^MarkRedisplayLine(y, xs, xe, isblank)$/
MarkRewrite	mark.c	/^MarkRewrite(ry, xs, xe, doit)$/
MarkRoutine	mark.c	/^MarkRoutine()$/
MarkScrollDownDisplay	mark.c	/^static int MarkScrollDownDisplay(n)$/
MarkScrollUpDisplay	mark.c	/^static int MarkScrollUpDisplay(n)$/
MarkSetCursor	mark.c	/^MarkSetCursor()$/
MoreWindows	process.c	/^MoreWindows()$/
Mscreen	screen.c	/^main(ac, av)$/
Msg	screen.c	/^Msg(int err, char *fmt, ...)$/
NewAutoFlow	ansi.c	/^NewAutoFlow(win, on)$/
NextWindow	process.c	/^NextWindow()$/
NukePending	display.c	/^NukePending()$/
OpenPTY	pty.c	/^OpenPTY(ttyn)$/
PUTCHAR	display.c	/^PUTCHAR(c)$/
PUTCHARLP	display.c	/^PUTCHARLP(c)$/
Panic	screen.c	/^Panic(int err, char *fmt, ...)$/
Parse	process.c	/^Parse(buf, args)$/
ParseChar	process.c	/^ParseChar(p, cp)$/
ParseEscape	process.c	/^ParseEscape(p)$/
ParseNum	process.c	/^ParseNum(act, var)$/
ParseOct	process.c	/^ParseOct(act, var)$/
ParseOnOff	process.c	/^ParseOnOff(act, var)$/
ParseSaveStr	process.c	/^ParseSaveStr(act, var)$/
ParseSwitch	process.c	/^ParseSwitch(act, var)$/
PreviousWindow	process.c	/^PreviousWindow()$/
PrintChar	ansi.c	/^PrintChar(c)$/
PrintFlush	ansi.c	/^PrintFlush()$/
ProcessInput	process.c	/^ProcessInput(ibuf, ilen)$/
PutChar	display.c	/^PutChar(c)$/
PutStr	display.c	/^PutStr(s)$/
RAW_PUTCHAR	display.c	/^RAW_PUTCHAR(c)$/
RCS_ID	rcs.h	/^#   define RCS_ID(id) static char *rcs_id() { retu/
RcLine	fileio.c	/^RcLine(ubuf)$/
ReadFile	fileio.c	/^ReadFile()$/
ReceiveMsg	socket.c	/^ReceiveMsg(s)$/
RecoverSocket	socket.c	/^RecoverSocket()$/
Redisplay	display.c	/^Redisplay(cur_only)$/
RefreshLine	display.c	/^RefreshLine(y, from, to, isblank)$/
RemoveLoginSlot	utmp.c	/^RemoveLoginSlot()$/
RemoveStatus	display.c	/^RemoveStatus()$/
RemoveUtmp	utmp.c	/^RemoveUtmp(wi)$/
Report	ansi.c	/^Report(fmt, n1, n2)$/
ResetWindow	ansi.c	/^ResetWindow(p)$/
ResizeDisplay	display.c	/^ResizeDisplay(wi, he)$/
ResizeHistArray	window.c	/^ResizeHistArray(p, arr, wi, hi, fillblank)$/
ResizeScreenArray	window.c	/^ResizeScreenArray(p, arr, wi, hi, fillblank)$/
Resize_obuf	display.c	/^Resize_obuf()$/
RestoreCursor	ansi.c	/^RestoreCursor()$/
RestoreLoginSlot	utmp.c	/^RestoreLoginSlot()$/
RestorePosAttrFont	ansi.c	/^RestorePosAttrFont()$/
Return	ansi.c	/^Return()$/
ReverseLineFeed	ansi.c	/^ReverseLineFeed()$/
S_ISFIFO	os.h	/^#define S_ISFIFO(mode) ((mode & S_IFMT) == S_IFIFO/
S_ISSOCK	os.h	/^#define S_ISSOCK(mode) ((mode & S_IFMT) == S_IFSOC/
SaveArgs	process.c	/^SaveArgs(args)$/
SaveChar	ansi.c	/^SaveChar(c)$/
SaveCursor	ansi.c	/^SaveCursor()$/
SaveStr	misc.c	/^SaveStr(str)$/
Scroll	ansi.c	/^Scroll(cp, cnt1, cnt2, tmp)$/
ScrollDownMap	ansi.c	/^ScrollDownMap(n)$/
ScrollRegion	display.c	/^ScrollRegion(ys, ye, n)$/
ScrollUpMap	ansi.c	/^ScrollUpMap(n)$/
Search	search.c	/^Search(dir)$/
SelectRendition	ansi.c	/^SelectRendition()$/
SendBreak	tty.c	/^void SendBreak(wp)$/
SendCreateMsg	socket.c	/^SendCreateMsg(s, nwin)$/
SendErrorMsg	socket.c	/^SendErrorMsg(char *fmt, ...)$/
SetAttr	display.c	/^SetAttr(new)$/
SetAttrFont	display.c	/^SetAttrFont(newattr, newcharset)$/
SetChar	ansi.c	/^SetChar(c)$/
SetCurr	ansi.c	/^SetCurr(wp)$/
SetFlow	tty.c	/^SetFlow(on)$/
SetFont	display.c	/^SetFont(new)$/
SetForeWindow	process.c	/^SetForeWindow(wi)$/
SetLastPos	display.c	/^SetLastPos(x,y)$/
SetMode	tty.c	/^SetMode(op, np)$/
SetTTY	tty.c	/^SetTTY(fd, mp)$/
SetUtmp	utmp.c	/^SetUtmp(wi)$/
ShowInfo	process.c	/^ShowInfo()$/
ShowTime	process.c	/^ShowTime()$/
ShowWindows	process.c	/^ShowWindows()$/
SigChld	screen.c	/^SigChld(SIGDEFARG)$/
SigChldHandler	screen.c	/^SigChldHandler()$/
SigHup	screen.c	/^SigHup(SIGDEFARG)$/
SigInt	screen.c	/^SigInt(SIGDEFARG)$/
SigStop	attacher.c	/^SigStop(SIGDEFARG)$/
SlotToggle	utmp.c	/^SlotToggle(how)$/
Special	ansi.c	/^Special(c)$/
StartRc	fileio.c	/^StartRc(rcfilename)$/
StartString	ansi.c	/^StartString(type)$/
SwitchWindow	process.c	/^SwitchWindow(n)$/
TtyGrabConsole	tty.c	/^TtyGrabConsole(fd, on, rc_name)$/
TtyNameSlot	utmp.c	/^TtyNameSlot(nam)$/
UserContext	misc.c	/^UserContext()$/
UserReturn	misc.c	/^UserReturn(val)$/
UserStatus	misc.c	/^UserStatus()$/
W2D	window.h	/^#define W2D(y) ((y) - markdata->hist_offset)$/
WEXITSTATUS	os.h	/^#  define WEXITSTATUS(status) ((status >> 8) & 037/
WIFCORESIG	os.h	/^#  define WIFCORESIG(status) (status & 0200)$/
WSTOPSIG	os.h	/^#  define WSTOPSIG(status) ((status >> 8) & 0377)$/
WTERMSIG	os.h	/^#  define WTERMSIG(status) (status & 0177)$/
WinClearLine	ansi.c	/^WinClearLine(y, xs, xe)$/
WinProcess	ansi.c	/^WinProcess(bufpp, lenp)$/
WinRedisplayLine	ansi.c	/^WinRedisplayLine(y, from, to, isblank)$/
WinResize	ansi.c	/^WinResize(wi, he)$/
WinRestore	ansi.c	/^WinRestore()$/
WinRewrite	ansi.c	/^WinRewrite(y, x1, x2, doit)$/
WinSetCursor	ansi.c	/^WinSetCursor()$/
WriteFile	fileio.c	/^WriteFile(dump)$/
WriteString	ansi.c	/^WriteString(wp, buf, len)$/
__P	screen.h	/^#  define __P(a) a$/
aWIN	window.h	/^#define aWIN(y) ((y < fore->histheight) ? fore->ah/
add_key_to_buf	help.c	/^add_key_to_buf(buf, key)$/
backsearchend	search.c	/^backsearchend(buf, len)$/
bclear	misc.c	/^bclear(p, n)$/
bcopy	misc.c	/^bcopy(s1, s2, len)$/
brktty	screen.c	/^brktty(fd)$/
bzero	os.h	/^# define bzero(poi,len) memset(poi,0,len)$/
centerline	misc.c	/^centerline(str)$/
chsock	socket.c	/^chsock()$/
closeallfiles	misc.c	/^closeallfiles()$/
copy_reg_fn	process.c	/^copy_reg_fn(buf, len)$/
copypage	help.c	/^copypage()$/
debug	screen.h	/^#	define debug(x) {fprintf(dfp,x);fflush(dfp);}$/
debug1	screen.h	/^#	define debug1(x,a) {fprintf(dfp,x,a);fflush(dfp)/
debug2	screen.h	/^#	define debug2(x,a,b) {fprintf(dfp,x,a,b);fflush(/
debug3	screen.h	/^#	define debug3(x,a,b,c) {fprintf(dfp,x,a,b,c);ffl/
display_copyright	help.c	/^display_copyright()$/
display_help	help.c	/^display_help()$/
e_tgetflag	termcap.c	/^e_tgetflag(cap)$/
e_tgetnum	termcap.c	/^e_tgetnum(cap)$/
e_tgetstr	termcap.c	/^e_tgetstr(cap, tepp)$/
eexit	screen.c	/^eexit(e)$/
eq	mark.c	/^eq(a, b)$/
execvpe	exec.c	/^void execvpe(prog, args, env)$/
exit_with_usage	help.c	/^exit_with_usage(myname)$/
expand_vars	fileio.c	/^expand_vars(ss)$/
fWIN	window.h	/^#define fWIN(y) ((y < fore->histheight) ? fore->fh/
fgtty	screen.c	/^fgtty(fd)$/
findacl	process.c	/^findacl(name)$/
findcap	termcap.c	/^findcap(cap, tepp, n)$/
findenv	putenv.c	/^static int  findenv(name)$/
findrcfile	fileio.c	/^findrcfile(rcfile)$/
freetty	screen.c	/^freetty()$/
getlogin	utmp.c	/^getlogin()$/
gettermcapstring	termcap.c	/^gettermcapstring(s)$/
getttyent	utmp.c	/^static struct ttyent *getttyent()$/
helppage	help.c	/^helppage()$/
iWIN	window.h	/^#define iWIN(y) ((y < fore->histheight) ? fore->ih/
if	utmp.c	/^  if (add_utmp(slot, &u) == -1)$/
inp_setprompt	input.c	/^inp_setprompt(p, s)$/
ins_reg_fn	process.c	/^ins_reg_fn(buf, len)$/
is_bm	search.c	/^is_bm(str, l, p, end, dir)$/
is_letter	mark.c	/^static int is_letter(c)$/
is_process	search.c	/^is_process(p, n)$/
is_redo	search.c	/^is_redo(markdata)$/
k_addr	loadav.c	/^k_addr(sym)$/
killpg	os.h	/^# define killpg(pgrp,sig) kill( -(pgrp), sig)$/
lineend	mark.c	/^lineend(y)$/
linestart	mark.c	/^linestart(y)$/
lseek	utmp.c	/^  (void) lseek(utmpf, (off_t) (slot * sizeof u), 0/
matchword	search.c	/^matchword(pattern, y, sx, ex)$/
moreenv	putenv.c	/^static int moreenv()$/
newenv	putenv.c	/^static int newenv()$/
nextword	mark.c	/^nextword(xp, yp, flags, num)$/
nwin_compose	tty.c	/^nwin_compose(def, new, res)$/
pass1	process.c	/^pass1(buf, len)$/
pass2	process.c	/^pass2(buf, len)$/
pow_detach_fn	process.c	/^pow_detach_fn(buf, len)$/
process_fn	process.c	/^process_fn(buf, len)$/
putenv	putenv.c	/^int putenv(string)$/
rem	mark.c	/^rem(x1, y1, x2, y2, redisplay, pt, yend)$/
revto	mark.c	/^void revto(tx, ty)$/
revto_line	mark.c	/^void revto_line(tx, ty, line)$/
sconnect	socket.c	/^sconnect(s, sapp, len)$/
screen_builtin_lck	attacher.c	/^screen_builtin_lck()$/
searchend	search.c	/^searchend(buf, len)$/
secfopen	fileio.c	/^secfopen(name, mode)$/
secopen	fileio.c	/^secopen(name, flags, mode)$/
setregid	os.h	/^# define setregid(rgid, egid) setresgid(rgid, egid/
setreuid	os.h	/^# define setreuid(ruid, euid) setresuid(ruid, euid/
setttyent	utmp.c	/^static void setttyent()$/
signal	misc.c	/^void (*signal(sig, func)) ()$/
stripdev	misc.c	/^stripdev(nam)$/
trysend	attacher.c	/^trysend(fd, m, pwto)$/
trysendfail	attacher.c	/^trysendfail(SIGDEFARG)$/
trysendok	attacher.c	/^trysendok(SIGDEFARG)$/
unsetenv	putenv.c	/^int unsetenv(name)$/
varcmp	loadav.c	/^varcmp(p1, p2)$/
winexec	exec.c	/^winexec(d, s)$/
xrealloc	window.c	/^xrealloc(mem, len)$/
