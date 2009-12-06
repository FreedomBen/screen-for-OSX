/* Copyright (c) 2008, 2009
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 *      Micah Cowan (micah@cowan.name)
 *      Sadrul Habib Chowdhury (sadrul@users.sourceforge.net)
 * Copyright (c) 1993-2002, 2003, 2005, 2006, 2007
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, see
 * http://www.gnu.org/licenses/, or contact Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 ****************************************************************
 */

/* Deals with the list of displays */

#ifdef MULTI

struct displaysdata
{
  struct display *selected;	/* The selected display */
  struct display *current;	/* The current display (that's showing this list) */
};

static void DisplaysProcess __P((char **, int *));
static void DisplaysRedisplayLine __P((int, int, int, int));
static void displayspage __P((struct displaysdata *));

static struct LayFuncs DisplaysLf =
{
  DisplaysProcess,
  HelpAbort,
  DisplaysRedisplayLine,
  DefClearLine,
  DefRewrite,
  DefResize,
  DefRestore
};

void
display_displays()
{
  struct displaysdata *ddata;
  if (flayer->l_width < 10 || flayer->l_height < 5)
    {
      LMsg(0, "Window size too small for displays page");
      return;
    }
  if (InitOverlayPage(sizeof(struct displaysdata), &DisplaysLf, 0))
    return;
  flayer->l_mode = 1;
  flayer->l_x = 0;
  flayer->l_y = flayer->l_height - 1;
  ddata = flayer->l_data;
  ddata->current = display;
  ddata->selected = display;
  displayspage(ddata);
}

static void
display_list_change(struct displaysdata *ddata, int dir)
{
  struct display *d;
  for (d = displays; d; d = d->d_next)
    {
      if (dir == -1 && d->d_next == ddata->selected)
	{
	  ddata->selected = d;
	  break;
	}
      else if (dir == 1 && d == ddata->selected && d->d_next)
	{
	  ddata->selected = d->d_next;
	  break;
	}
    }

  if (d)
    {
      /* Selection changed */
      displayspage(ddata);
    }
}

static void
DisplaysProcess(ppbuf, plen)
char **ppbuf;
int *plen;
{
  int done = 0;

  ASSERT(flayer);
  while (!done && *plen > 0)
    {
      unsigned char ch = (unsigned char)**ppbuf;
      ++*ppbuf;
      --*plen;
      if (flayer->l_mouseevent.start)
	{
	  int r = LayProcessMouse(flayer, ch);
	  if (r == -1)
	    LayProcessMouseSwitch(flayer, 0);
	  else
	    {
	      if (r)
		ch = 0222;
	      else
		continue;
	    }
	}
      switch (ch)
	{
	case ' ':
	  displayspage(flayer->l_data);
	  break;
	case '\r':
	case '\n':
	  HelpAbort();
	  done = 1;
	  break;

	case 0220:	/* up */
	case 16:	/* ^P like emacs */
	case 'k':
	  display_list_change(flayer->l_data, -1);
	  break;

	case 0216:	/* down */
	case 14:	/* ^N like emacs */
	case 'j':
	  display_list_change(flayer->l_data, 1);
	  break;

	case 0222:	/* mouse event */
	  if (flayer->l_mouseevent.start)
	    {
	      /* All the data is available to process the mouse event. */
	      int button = flayer->l_mouseevent.buffer[0];
	      if (button == 'a')
		{
		  /* Scroll down */
		  display_list_change(flayer->l_data, 1);
		}
	      else if (button == '`')
		{
		  /* Scroll up */
		  display_list_change(flayer->l_data, -1);
		}
	      LayProcessMouseSwitch(flayer, 0);
	    }
	  else
	    LayProcessMouseSwitch(flayer, 1);
	  break;

	default:
	  break;
	}
    }
}


/*
 * layout of the displays page is as follows:

xterm 80x42      jnweiger@/dev/ttyp4    0(m11)    &rWx
facit 80x24 nb   mlschroe@/dev/ttyhf   11(tcsh)    rwx
xterm 80x42      jnhollma@/dev/ttyp5    0(m11)    &R.x

  |     |    |      |         |         |   |     | ¦___ window permissions 
  |     |    |      |         |         |   |     |      (R. is locked r-only,
  |     |    |      |         |         |   |     |       W has wlock)
  |     |    |      |         |         |   |     |___ Window is shared
  |     |    |      |         |         |   |___ Name/Title of window
  |     |    |      |         |         |___ Number of window
  |     |    |      |         |___ Name of the display (the attached device)
  |     |    |      |___ Username who is logged in at the display
  |     |    |___ Display is in nonblocking mode. Shows 'NB' if obuf is full.
  |     |___ Displays geometry as width x height.
  |___ the terminal type known by screen for this display.
 
 */

static void
displayspage(ddata)
struct displaysdata *ddata;
{
  int y, l;
  char tbuf[80];
  struct display *d;
  struct win *w;
  static char *blockstates[5] = {"nb", "NB", "Z<", "Z>", "BL"};
  struct mchar m_current = mchar_blank;
  m_current.attr = A_BD;

  if (!ddata)
    return;

  LClearAll(flayer, 0);

  leftline("term-type   size         user interface           window       Perms", 0, 0);
  leftline("---------- ------- ---------- ----------------- ----------     -----", 1, 0);
  y = 2;

  for (d = displays; d; d = d->d_next)
    {
      struct mchar *mc;
      w = d->d_fore;

      if (y >= flayer->l_height - 3)
	break;
      sprintf(tbuf, "%-10.10s%4dx%-4d%10.10s@%-16.16s%s",
	      d->d_termname, d->d_width, d->d_height, d->d_user->u_name,
	      d->d_usertty,
	      (d->d_blocked || d->d_nonblock >= 0) && d->d_blocked <= 4 ? blockstates[d->d_blocked] : "  ");

      if (w)
	{
	  l = 10 - strlen(w->w_title);
	  if (l < 0)
	    l = 0;
	  sprintf(tbuf + strlen(tbuf), "%3d(%.10s)%*s%c%c%c%c",
		  w->w_number, w->w_title, l, "",
		  /* w->w_dlist->next */ 0 ? '&' : ' ',
		  /*
		   * The rwx triple:
		   * -,r,R	no read, read, read only due to foreign wlock
		   * -,.,w,W	no write, write suppressed by foreign wlock,
		   *            write, own wlock
		   * -,x	no execute, execute
		   */
#ifdef MULTIUSER
		  (AclCheckPermWin(d->d_user, ACL_READ, w) ? '-' :
		   ((w->w_wlock == WLOCK_OFF || d->d_user == w->w_wlockuser) ?
		    'r' : 'R')),
		  (AclCheckPermWin(d->d_user, ACL_READ, w) ? '-' :
		   ((w->w_wlock == WLOCK_OFF) ? 'w' :
		    ((d->d_user == w->w_wlockuser) ? 'W' : 'v'))),
		  (AclCheckPermWin(d->d_user, ACL_READ, w) ? '-' : 'x')
#else
		  'r', 'w', 'x'
#endif
	  );
	}
      leftline(tbuf, y, d == ddata->selected ? &mchar_so : d == ddata->current ? &m_current : 0);
      y++;
    }
  sprintf(tbuf,"[Press Space %s Return to end.]",
	  1 ? "to refresh;" : "or");
  centerline(tbuf, flayer->l_height - 2);
  LaySetCursor();
}

static void
DisplaysRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  ASSERT(flayer);
  if (y < 0)
    {
      displayspage(flayer->l_data);
      return;
    }
  if (y != 0 && y != flayer->l_height - 1)
    return;
  if (isblank)
    return;
  LClearArea(flayer, xs, y, xe, y, 0, 0);
  /* To be filled in... */
}

#endif /* MULTI */

