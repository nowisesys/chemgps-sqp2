bin_PROGRAMS = cgpsddos
cgpsddos_SOURCES = ../../cgpsclt/result.c ../../cgpsclt/request.c \
		   ../../cgpsclt/socket.c main.c options.c cgpsddos.h  \
		   master.c slave.c local.c bind.c resolve.c dgram.c \
		   dllist.c  dllist.h connect.c 

cgpsddos_CFLAGS  = -I$(top_srcdir)/libcgpssqp -I$(top_srcdir)/$(LIBCHEMGPS_INCDIR) -I$(top_srcdir)/cgpsclt

cgpsddos_LDADD   = $(top_srcdir)/libcgpssqp/libcgpssqp.a
