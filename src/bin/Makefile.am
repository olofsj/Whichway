AUTOMAKE_OPTIONS     = 1.4 foreign

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/bin \
-I$(top_srcdir)/src/lib \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-DPACKAGE_BIN_DIR=\"$(bindir)\"

bin_PROGRAMS = whichway_create_index whichway_test

whichway_create_index_SOURCES = whichway_create_index.c
whichway_create_index_LDADD = -lexpat -lm $(top_builddir)/src/lib/libwhichway.la
whichway_create_index_LDFLAGS =

whichway_test_SOURCES = whichway_test.c
whichway_test_LDADD = -lm $(top_builddir)/src/lib/libwhichway.la
whichway_test_LDFLAGS =

