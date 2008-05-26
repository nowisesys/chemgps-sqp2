bin_PROGRAMS = cgpsddos
cgpsddos_SOURCES = ../../cgpsclt/result.c ../../cgpsclt/request.c \
		   ../../cgpsclt/socket.c main.c options.c cgpsddos.h  \
		   master.c slave.c local.c bind.c resolve.c dgram.c \
		   connect.c signal.c

cgpsddos_CFLAGS  = -I$(top_srcdir)/libcgpssqp -I$(top_srcdir)/cgpsclt -I$(SIMCAQ_INCDIR) 
cgpsddos_LDADD   = $(top_srcdir)/libcgpssqp/libcgpssqp.a

## if LIBCHEMGPS_DEVELOP
##  cgpsddos_CFLAGS += -I$(top_srcdir)/$(LIBCHEMGPS_INCDIR)
## endif
## if LIBCHEMGPS_STDINC
##   cgpsddos_CFLAGS +=
## else
##   cgpsddos_CFLAGS += -I$(LIBCHEMGPS_INCDIR)
## endif
