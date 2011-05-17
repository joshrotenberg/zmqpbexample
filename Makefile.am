ACLOCAL_AMFLAGS = -I m4
SUBDIRS = . src

MAINTAINERCLEANFILES = \
	$(srcdir)/aclocal.m4            \
	$(srcdir)/autom4te.cache        \
	$(srcdir)/configure             \
	$(srcdir)/zmqpbexample.pb.cc	\
	$(srcdir)/zmqpbexample.pb.h	\
	`find "$(srcdir)" -type f -name Makefile.in -print`

maintainer-clean-local:
	-rm -rf $(top_srcdir)/config
