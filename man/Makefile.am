if HAVE_HELP2MAN

HELP2MAN=help2man -N
SUFFIXES = .1

gfcat.1: $(top_builddir)/build/bin/gfcat
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfcli.1: $(top_builddir)/build/bin/gfcli
	$(HELP2MAN) --output=$@-t $(top_builddir)/build/bin/$* \
		&& mv $@-t $@

gfcp.1: $(top_builddir)/build/bin/gfcp
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfls.1: $(top_builddir)/build/bin/gfls
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfmkdir.1: $(top_builddir)/build/bin/gfmkdir
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@


gfput.1: $(top_builddir)/build/bin/gfput
	$(HELP2MAN) --output=$@-t $(top_builddir)/build/bin/$* \
		&& mv $@-t $@

gfrm.1: $(top_builddir)/build/bin/gfrm
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfstat.1: $(top_builddir)/build/bin/gfstat
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gftail.1: $(top_builddir)/build/bin/gftail
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gftruncate.1: $(top_builddir)/build/bin/gftruncate
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gftouch.1: $(top_builddir)/build/bin/gftouch
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

gfrmdir.1: $(top_builddir)/build/bin/gfrmdir
	$(HELP2MAN) --output=$@-t -I common_seealso.h2m \
		$(top_builddir)/build/bin/$* && mv $@-t $@

man1_MANS = gfcat.1 \
	    gfcli.1 \
	    gfcp.1 \
	    gfls.1 \
	    gfmkdir.1 \
	    gfput.1 \
	    gfrm.1 \
	    gfstat.1 \
	    gftail.1 \
	    gftruncate.1 \
	    gftouch.1 \
	    gfrmdir.1

EXTRA_DIST = common_seealso.h2m

$(man1_MANS): $(srcdir)/common_seealso.h2m

CLEANFILES = $(man1_MANS)

endif
