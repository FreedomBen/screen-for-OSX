char cpmsg[] = "\
\n\
iScreen version %v\n\
\n\
Copyright (c) 1991\n\
    Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)\n\
    Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)\n\
Copyright (c) 1987 Oliver Laumann\n\
\n\
This program is free software; you can redistribute it and/or \
modify it under the terms of the GNU General Public License as published \
by the Free Software Foundation; either version 1, or (at your option)\n\
any later version.\n\
\n\
This program is distributed in the hope that it will be useful, \
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the \
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License \
along with this program (see the file COPYING); if not, write to the \
Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n";

char version[] = "VERSIONTEST";

static char *s, *saveds;
int screenwidth;

main(argc, argv)
int argc;
char **argv;
{
  screenwidth = atoi(argv[1]);
  s = cpmsg;
  while (*s)
    {
      prcpy();
      printf("============================\n");
    }
}

prcpy()
{
  char *ws;
  int x, y, l;
  x = y = 0;
  
  while(*s)
    {
      ws = s;
      while (*s == ' ')
	s++;
      if (strncmp(s, "%v", 2) == 0)
	{
	  saveds = s + 2;
	  ws = s = version;
	}
      while (*s && *s != ' ' && *s != '\n')
	s++;
      l = s - ws;
      s = ws;
      if (l > screenwidth - 1)
	l = screenwidth - 1;
      if (x && x + l >= screenwidth - 2)
	{
	  putchar('\n');
	  x = 0;
	  if (++y == 10)
            break;
	}
      if (x)
	{
	  putchar(' ');
	  x++;
	}
      printf("%*.*s", l, l, ws);
      x += l;
      s += l;
      if (*s == 0 && saveds)
	{
	  s = saveds;
	  saveds = 0;
	}
      if (*s == '\n')
	{
	  putchar('\n');
	  x = 0;
	  if (++y == 10)
            break;
	}
      if (*s == ' ' || *s == '\n')
	s++;
    }
  while (y++ != 10)
    putchar('\n');
}
  
