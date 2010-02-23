/* Copyright (c) 2010
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Sadrul Habib Chowdhury (sadrul@users.sourceforge.net)
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

/* Deals with the list of windows */

#include "config.h"
#include "screen.h"
#include "layer.h"
#include "extern.h"
#include "list_generic.h"

extern struct layer *flayer;
extern struct display *display;

extern char *wlisttit;
extern char *wliststr;

extern struct mchar mchar_blank, mchar_so;
extern int renditions[];

extern struct win **wtab, *windows;
extern int maxwin;

struct gl_Window_Data
{
  struct win *group;	/* Currently displaying this group */
  int order;		/* MRU? NESTED? etc. */
  int onblank;
  struct win *fore;	/* The foreground window we had. */
};

static void
gl_Window_rebuild(struct ListData *ldata)
{
  struct ListRow *row = NULL;
  struct gl_Window_Data *wdata = ldata->data;

  if (wdata->order == WLIST_MRU)
    {
      struct win *w;
      for (w = windows; w; w = w->w_next)
	{
	  if (w->w_group == wdata->group)
	    {
	      row = glist_add_row(ldata, w, row);
	      if (w == wdata->fore)
		ldata->selected = row;
	    }
	}
    }
  else
    {
      struct win **w;
      struct win *wlist;
      for (w = wtab, wlist = windows; wlist && w - wtab < maxwin; w++)
	{
	  if (*w && (*w)->w_group == wdata->group)
	    {
	      row = glist_add_row(ldata, *w, row);
	      if (*w == wdata->fore)
		ldata->selected = row;
	      wlist = wlist->w_next;
	    }
	}
    }
  glist_display_all(ldata);
}

static int
gl_Window_header(struct ListData *ldata)
{
  char *str;

  display = 0;
  str = MakeWinMsgEv(wlisttit, (struct win *)0, '%', flayer->l_width, (struct event *)0, 0);

  LPutWinMsg(flayer, str, strlen(str), &mchar_blank, 0, 0);
  return 2;
}

static int
gl_Window_footer(struct ListData *ldata)
{
  return 0;
}

static int
gl_Window_row(struct ListData *ldata, struct ListRow *lrow)
{
  char *str;
  struct win *w;
  int xoff;
  struct mchar *mchar;
  struct mchar mchar_rend = mchar_blank;
  struct ListRow *par = lrow;

  w = lrow->data;

  /* XXX: First, make sure we want to display this window in the list.
   * If we are showing a list for a group, and not on blank, then we must
   * only show the windows directly belonging to that group.
   * Otherwise, do some more checks. */

  for (xoff = 0, par = lrow->parent; par; par = par->parent)
    {
      if (par->y == -1)
	break;
      xoff += 2;
    }
  display = Layer2Window(flayer) ? 0 : flayer->l_cvlist ? flayer->l_cvlist->c_display : 0;
  str = MakeWinMsgEv(wliststr, w, '%', flayer->l_width - xoff, NULL, 0);
  if (ldata->selected == lrow)
    mchar = &mchar_so;
  else if (w->w_monitor == MON_DONE && renditions[REND_MONITOR] != -1)
    {
      mchar = &mchar_rend;
      ApplyAttrColor(renditions[REND_MONITOR], mchar);
    }
  else if ((w->w_bell == BELL_DONE || w->w_bell == BELL_FOUND) && renditions[REND_BELL] != -1)
    {
      mchar = &mchar_rend;
      ApplyAttrColor(renditions[REND_BELL], mchar);
    }
  else
    mchar = &mchar_blank;

  LPutWinMsg(flayer, str, flayer->l_width, mchar, xoff, lrow->y);
  return 1;
}

static int
gl_Window_input(struct ListData *ldata, char **inp, int *len)
{
  struct win *win;
  unsigned char ch;
  struct display *cd = display;
  struct gl_Window_Data *wdata = ldata->data;

  if (!ldata->selected)
    return 0;

  ch = (unsigned char) **inp;
  ++*inp;
  --*len;

  switch (ch)
    {
    case ' ':
    case '\n':
    case '\r':
      win = ldata->selected->data;
      if (!win)
	break;
#ifdef MULTIUSER
      if (display && AclCheckPermWin(D_user, ACL_READ, win))
	return;		/* Not allowed to switch to this window. */
#endif
      glist_abort();
      display = cd;
      SwitchWindow(win->w_number);
      break;

    case 033:	/* escape */
    case 007:	/* ^G */
      if (wdata->onblank)
	{
	  glist_abort();
	  display = cd;
	  SwitchWindow(wdata->fore->w_number);
	  *len = 0;
	  break;
	}
      /* else FALLTHROUGH */
    default:
      --*inp;
      ++*len;
      return 0;
    }
  return 1;
}

static int
gl_Window_freerow(struct ListData *ldata, struct ListRow *row)
{
  return 0;
}

static int
gl_Window_free(struct ListData *ldata)
{
  Free(ldata->data);
  return 0;
}

static struct GenericList gl_Window =
{
  gl_Window_header,
  gl_Window_footer,
  gl_Window_row,
  gl_Window_input,
  gl_Window_freerow,
  gl_Window_free
};

void
display_windows(int onblank, int order, struct win *group)
{
  struct win *p;
  struct wlistdata *wlistdata;

  if (flayer->l_width < 10 || flayer->l_height < 6)
    {
      LMsg(0, "Window size too small for window list page");
      return;
    }
  if (onblank)
    {
      debug3("flayer %x %d %x\n", flayer, flayer->l_width, flayer->l_height);
      if (!display)
	{
	  LMsg(0, "windowlist -b: display required");
	  return;
	}
      p = D_fore;
      if (p)
	{
	  SetForeWindow((struct win *)0);
          if (p->w_group)
	    {
	      D_fore = p->w_group;
	      flayer->l_data = (char *)p->w_group;
	    }
	  Activate(0);
	}
      if (flayer->l_width < 10 || flayer->l_height < 6)
	{
	  LMsg(0, "Window size too small for window list page");
	  return;
	}
    }
  else
    p = Layer2Window(flayer);
  if (!group && p)
    group = p->w_group;

  struct ListData *ldata;
  struct gl_Window_Data *wdata;
  ldata = glist_display(&gl_Window);
  if (!ldata)
    {
      if (onblank && p)
	{
	  /* Could not display the list. So restore the window. */
	  SetForeWindow(p);
	  Activate(1);
	}
      return;
    }

  wdata = calloc(1, sizeof(struct gl_Window_Data));
  wdata->group = group;
  wdata->order = order;
  wdata->onblank = onblank;
  wdata->fore = p;
  ldata->data = wdata;

  gl_Window_rebuild(ldata);
}

