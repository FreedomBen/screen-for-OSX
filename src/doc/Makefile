INSTALL = cp
infodir = /local/emacs/info

all: screen.dvi screen.info

dvi screen.dvi: screen.texinfo clean
	texi2dvi screen.texinfo

info screen.info: screen.texinfo
	makeinfo screen.texinfo

install: screen.info
	$(INSTALL) screen.info $(infodir)/screen
	$(INSTALL) screen.info-? $(infodir)/
	
clean:
	-rm -f *.cp *.cps *.fn *.fns *.ky *.kys *.pg *.tp *.vr
	-rm -f *.log *.aux *.toc *~

distclean realclean clobber: clean
	-rm -f *.dvi
	-rm -f *.info *.info-?

