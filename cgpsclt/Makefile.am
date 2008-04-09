## The cgpsclt target can be built without linking against libchemgps (foreign
## mode). This allowes the cgpsclt application to run on systems without
## the libsimcaq library installed.

bin_PROGRAMS = cgpsclt
cgpsclt_SOURCES = main.c options.c socket.c cgpsclt.h request.c  
cgpsclt_LDADD = ../libcgpssqp/libcgpssqp.a
cgpsclt_CFLAGS = -I../libcgpssqp
if FOREIGN_CLIENT
  cgpsclt_SOURCES += result.c
else
  cgpsclt_LDADD += -lchemgps
endif
