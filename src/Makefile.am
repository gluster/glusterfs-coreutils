AM_CPPFLAGS = $(GF_CPPFLAGS) -I$(top_builddir)/lib -I$(top_srcdir)/lib
LDADD = $(top_builddir)/lib/libgnu.a

all-local:
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gfcat
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gfcp
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gfls
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gfmkdir
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gftouch
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gfrm
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gfstat
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gftail
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gftruncate
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gfrmdir
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gfclear
	$(LN_S) -f gfcli $(top_builddir)/build/bin/gfmv

bin_PROGRAMS = 	$(top_builddir)/build/bin/gfcli \
		$(top_builddir)/build/bin/gfput

EXTRA_DIST = glfs-cat.h \
	     glfs-cp.h \
	     glfs-cli-commands.h \
	     glfs-cli.h \
	     glfs-flock.h \
	     glfs-ls.h \
	     glfs-mkdir.h \
	     glfs-touch.h \
	     glfs-rm.h \
	     glfs-stat.h \
	     glfs-stat-util.h \
	     glfs-tail.h \
	     glfs-util.h \
	     glfs-truncate.h \
	     glfs-rmdir.h \
	     glfs-clear.h \
	     glfs-mv.h

__top_builddir__build_bin_gfcli_SOURCES = glfs-cli.c \
					  glfs-cli-commands.c \
					  glfs-cat.c \
					  glfs-cp.c \
					  glfs-flock.c \
					  glfs-ls.c \
					  glfs-mkdir.c \
					  glfs-touch.c \
					  glfs-rm.c \
					  glfs-stat.c \
					  glfs-stat-util.c \
					  glfs-tail.c \
					  glfs-util.c \
					  glfs-truncate.c \
					  glfs-rmdir.c \
					  glfs-clear.c \
					  glfs-mv.c

__top_builddir__build_bin_gfcli_CFLAGS = $(GLFS_CFLAGS)
__top_builddir__build_bin_gfcli_LDADD = $(LDADD) $(GLFS_LIBS) -lreadline

__top_builddir__build_bin_gfput_SOURCES = glfs-put.c glfs-util.c
__top_builddir__build_bin_gfput_CFLAGS = $(GLFS_CFLAGS)
__top_builddir__build_bin_gfput_LDADD = $(LDADD) $(GLFS_LIBS)
