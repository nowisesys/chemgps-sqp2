bin_PROGRAMS = cgpsstd
cgpsstd_SOURCES = main.c options.c

cgpsstd_CFLAGS = -I../libcgpssqp -I$(SIMCAQ_INCDIR)

cgpsstd_LDFLAGS = -L$(SIMCAQ_LIBDIR)
cgpsstd_LDADD = -lchemgps -lsimcaq ../libcgpssqp/libcgpssqp.a
