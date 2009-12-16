AUTOMAKE_OPTIONS     = 1.4 foreign

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/lib \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-DPACKAGE_BIN_DIR=\"$(bindir)\"

lib_LTLIBRARIES = libwhichway.la

include_HEADERS = Whichway.h

libwhichway_la_SOURCES = whichway_utils.c whichway_routing_astar.c

libwhichway_la_CFLAGS =
libwhichway_la_LIBADD = 
libwhichway_la_LDFLAGS = -no-undefined 

