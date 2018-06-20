ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib src man

EXTRA_DIST = m4/gnulib-cache.m4

install-exec-local:
	$(LN_S) -f gfcli $(DESTDIR)$(bindir)/gfcat
	$(LN_S) -f gfcli $(DESTDIR)$(bindir)/gfcp
	$(LN_S) -f gfcli $(DESTDIR)$(bindir)/gfls
	$(LN_S) -f gfcli $(DESTDIR)$(bindir)/gfmkdir
	$(LN_S) -f gfcli $(DESTDIR)$(bindir)/gfrm
	$(LN_S) -f gfcli $(DESTDIR)$(bindir)/gfstat
	$(LN_S) -f gfcli $(DESTDIR)$(bindir)/gftail
	$(LN_S) -f gfcli $(DESTDIR)$(bindir)/gfmv

rpms:	prep
	QA_RPATHS=17 rpmbuild --define '_topdir $(abs_top_builddir)/build/rpmbuild' \
		-ba $(abs_top_builddir)/build/rpmbuild/SPECS/glusterfs-coreutils.spec

prep:	dist
	mkdir -p build/rpmbuild && cd build/rpmbuild \
		&& mkdir BUILD SPECS RPMS SRPM SOURCES && cd ../..
	rm -rf rpmbuild/SOURCES/*
	cp *.tar.gz build/rpmbuild/SOURCES
	cp glusterfs-coreutils.spec build/rpmbuild/SPECS

uninstall-local:
	cd $(DESTDIR)$(bindir) && rm -f gfcat gfcp gfls gfmkdir gfmv gfrm gfstat gftail

clean-local:
	rm -rf build rpmbuild *.rpm

gitclean: clean distclean
	find . -path gnulib -prune -name Makefile.in -exec rm -f {} \;
	find . -path gnulib -prune -name Makefile -exec rm -f {} \;
	rm -f aclocal.m4 compile config.* configure depcomp install-sh libtool \
		ltmain.sh missing stamp-h1
	rm -rf autom4te.cache
