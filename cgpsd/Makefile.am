sbin_PROGRAMS = cgpsd
cgpsd_SOURCES = main.c options.c server.c socket.c cgpsd.h client.c signal.c \
	        worker.c worker.h

cgpsd_CFLAGS  = -I../libcgpssqp -I$(SIMCAQ_INCDIR)

cgpsd_LDFLAGS = -L$(SIMCAQ_LIBDIR)
cgpsd_LDADD   = ../libcgpssqp/libcgpssqp.a -lchemgps -lsimcaq
