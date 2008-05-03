sbin_PROGRAMS = cgpsd
cgpsd_SOURCES = main.c options.c server.c socket.c cgpsd.h client.c signal.c \
	        worker.c worker.h

cgpsd_CFLAGS  = -I../libcgpssqp -I$(top_srcdir)/$(LIBCHEMGPS_INCDIR) -I$(SIMCAQ_INCDIR)

cgpsd_LDFLAGS = -L$(top_srcdir)/$(LIBCHEMGPS_LIBDIR) -L$(SIMCAQ_LIBDIR)
cgpsd_LDADD   = ../libcgpssqp/libcgpssqp.a -lchemgps -lsimcaq
