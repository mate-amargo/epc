prefix = /usr
datadir = /usr/share/epc
AM_CFLAGS = -DDATADIR=\"$(datadir)\"
bin_PROGRAMS = epcc
bin_SCRIPTS = epcf
epcc_SOURCES = main.c
man_MANS = epc.1
functionsdir = $(datadir)/functions
functions_DATA = functions/battplug functions/battstate functions/exitface \
                 functions/fdate functions/files functions/filesall \
                 functions/fsize functions/fsizeall functions/fuptime \
                 functions/tdir functions/term functions/termfull \
                 functions/users
rcdir = $(datadir)
rc_DATA = epcrc
EXTRA_DIST = $(man_MANS) $(bin_SCRIPTS) $(functions_DATA) $(rc_DATA)
