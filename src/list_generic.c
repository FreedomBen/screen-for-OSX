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

#include "config.h"
#include "screen.h"
#include "list_generic.h"
#include "layer.h"

/* Deals with a generic list display */

extern struct layer *flayer;

static void ListProcess __P((char **, int *));
static void ListAbort __P((void));
static void ListRedisplayLine __P((int, int, int, int));
static void ListClearLine __P((int, int, int, int));
static int  ListRewrite __P((int, int, int, struct mchar *, int));
static int  ListResize __P((int, int));
static void ListRestore __P((void));

struct LayFuncs ListLf =
{
  ListProcess,
  ListAbort,
  ListRedisplayLine,
  ListClearLine,
  ListRewrite,
  ListResize,
  ListRestore
};

/** Returns non-zero on success. */
struct ListData *
glist_display(struct GenericList *list, const char *name)
{
  struct ListData *ldata;

  if (InitOverlayPage(sizeof(struct ListData), &ListLf, 0))
    return NULL;
  ldata = flayer->l_data;

  ldata->name = name;	/* We do not SaveStr, since the strings should be all static literals */
  ldata->list_fn = list;

  flayer->l_mode = 1;
  flayer->l_x = 0;
  flayer->l_y = flayer->l_height - 1;

  return ldata;
}

static void ListProcess(char **ppbuf, int *plen)
{
  struct ListData *ldata = flayer->l_data;

  while (*plen > 0)
    {
      struct ListRow *old;
      unsigned char ch;

      if (ldata->list_fn->gl_pinput &&
	  ldata->list_fn->gl_pinput(ldata, ppbuf, plen))
	continue;

      if (!ldata->selected)
	{
	  *plen = 0;
	  break;
	}

      ch = **ppbuf;
      ++*ppbuf;
      --*plen;

      old = ldata->selected;
      switch (ch)
	{
	case ' ':
	  break;

	case '\r':
	case '\n':
	  break;

	case 0220:	/* up */
	case 16:	/* ^P */
	case 'k':
	  if (!ldata->selected->prev)	/* There's no where to go */
	    break;
	  ldata->selected = old->prev;
	  break;

	case 0216:	/* down */
	case 14:	/* ^N like emacs */
	case 'j':
	  if (!ldata->selected->next)	/* Nothing to do */
	    break;
	  old = ldata->selected;
	  ldata->selected = old->next;
	  break;

	case 033:	/* escape */
	case 007:	/* ^G */
	  ListAbort();
	  *plen = 0;
	  return;
	}

      if (old == ldata->selected)	/* The selection didn't change */
	continue;

      if (ldata->selected->y == -1)
	{
	  /* We need to list all the rows, since we are scrolling down */
	  glist_display_all(ldata);
	}
      else
	{
	  /* just redisplay the two lines. */
	  ldata->list_fn->gl_printrow(ldata, old);
	  ldata->list_fn->gl_printrow(ldata, ldata->selected);
	  flayer->l_y = ldata->selected->y;
	  LaySetCursor();
	}
    }
}

static void ListAbort(void)
{
  struct ListData *ldata = flayer->l_data;
  glist_remove_rows(ldata);
  if (ldata->list_fn->gl_free)
    ldata->list_fn->gl_free(ldata);
  LAY_CALL_UP(LRefreshAll(flayer, 0));
  ExitOverlayPage();
}

static void ListRedisplayLine(int y, int xs, int xe, int isblank)
{
  ASSERT(flayer);
  if (y < 0)
    {
      glist_display_all(flayer->l_data);
      return;
    }
  if (y != 0 && y != flayer->l_height - 1)
    return;

  if (isblank)
    return;
  LClearArea(flayer, xs, y, xe, y, 0, 0);
}

static void ListClearLine(int y, int xs, int xe, int bce)
{
  DefClearLine(y, xs, xe, bce);
}

static int  ListRewrite(int y, int xs, int xe, struct mchar *rend, int doit)
{
  return EXPENSIVE;
}

static int ListResize (int wi, int he)
{
  if (wi < 10 || he < 5)
    return -1;

  flayer->l_width = wi;
  flayer->l_height = he;
  flayer->l_y = he - 1;

  return 0;
}

static void ListRestore (void)
{
  DefRestore();
}

struct ListRow *
glist_add_row(struct ListData *ldata, void *data, struct ListRow *after)
{
  struct ListRow *r = calloc(1, sizeof(struct ListRow));
  r->data = data;

  if (after)
    {
      r->next = after->next;
      r->prev = after;
      after->next = r;
      if (r->next)
	r->next->prev = r;
    }
  else
    {
      r->next = ldata->root;
      if (ldata->root)
	ldata->root->prev = r;
      ldata->root = r;
    }

  return r;
}

void
glist_remove_rows(struct ListData *ldata)
{
  struct ListRow *row;
  for (row = ldata->root; row; row = row->next)
    {
      ldata->list_fn->gl_freerow(ldata, row);
      free(row);
    }
  ldata->root = ldata->selected = ldata->top = NULL;
}

void
glist_display_all(struct ListData *list)
{
  int y;
  struct ListRow *row;

  LClearAll(flayer, 0);

  y = list->list_fn->gl_printheader(list);

  if (!list->top)
    list->top = list->root;
  if (!list->selected)
    list->selected = list->root;

  for (row = list->root; row != list->top; row = row->next)
    row->y = -1;

  for (row = list->top; row; row = row->next)
    {
      row->y = y++;
      if (!list->list_fn->gl_printrow(list, row))
	{
	  row->y = -1;
	  y--;
	}
      if (y + 1 == flayer->l_height)
	break;
    }
  for (; row; row = row->next)
    row->y = -1;

  list->list_fn->gl_printfooter(list);
  if (list->selected && list->selected->y != -1)
    flayer->l_y = list->selected->y;
  else
    flayer->l_y = flayer->l_height - 1;
  LaySetCursor();
}

void glist_abort(void)
{
  ListAbort();
}

