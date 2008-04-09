bin_PROGRAMS = stress
stress_SOURCES = main.c options.c socket.c stress.h request.c result.c

stress_CFLAGS  = -I../../libcgpssqp -I$(top_srcdir)/$(LIBCHEMGPS_INCDIR)
stress_LDADD   = ../../libcgpssqp/libcgpssqp.a
