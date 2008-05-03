bin_PROGRAMS = cgpsstd
cgpsstd_SOURCES = main.c options.c

cgpsstd_CFLAGS = -I../libcgpssqp -I$(top_srcdir)/$(LIBCHEMGPS_INCDIR) -I$(SIMCAQ_INCDIR)

cgpsstd_LDFLAGS = -L$(top_srcdir)/$(LIBCHEMGPS_LIBDIR) -L$(SIMCAQ_LIBDIR)
cgpsstd_LDADD = -lchemgps -lsimcaq ../libcgpssqp/libcgpssqp.a
