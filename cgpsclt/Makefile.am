## The cgpsclt target can be built without linking against libchemgps (foreign
## mode). This allowes the cgpsclt application to run on systems without
## the libsimcaq library installed.

bin_PROGRAMS = cgpsclt
cgpsclt_SOURCES = main.c options.c socket.c cgpsclt.h request.c  

cgpsclt_CFLAGS = -I../libcgpssqp -I$(top_srcdir)/$(LIBCHEMGPS_INCDIR)
cgpsclt_LDADD = ../libcgpssqp/libcgpssqp.a

if FOREIGN_CLIENT
  cgpsclt_SOURCES += result.c
else
  cgpsclt_LDFLAGS = -L$(top_srcdir)/$(LIBCHEMGPS_LIBDIR)
  cgpsclt_LDADD += -lchemgps
endif
