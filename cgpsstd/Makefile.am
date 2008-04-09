bin_PROGRAMS = cgpsstd
cgpsstd_SOURCES = main.c options.c

cgpsstd_CFLAGS = -I../libcgpssqp -I$(top_srcdir)/$(LIBCHEMGPS_INCDIR)

cgpsstd_LDFLAGS = -L$(top_srcdir)/$(LIBCHEMGPS_LIBDIR)
cgpsstd_LDADD = -lchemgps ../libcgpssqp/libcgpssqp.a
