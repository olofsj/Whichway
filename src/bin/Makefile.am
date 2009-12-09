AUTOMAKE_OPTIONS     = 1.4 foreign

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/bin \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-I/usr/include/postgresql

bin_PROGRAMS = whichway_create_index

whichway_create_index_SOURCES = whichway_create_index.c

whichway_create_index_LDADD = -lpq
whichway_create_index_LDFLAGS =

