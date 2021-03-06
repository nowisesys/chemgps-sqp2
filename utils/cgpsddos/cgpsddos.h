/* SIMCA-QP predictions for the ChemGPS project.
 *
 * Copyright (C) 2007-2018 Anders Lövgren and the Computing Department,
 * Uppsala Biomedical Centre, Uppsala University.
 * 
 * Copyright (C) 2018-2019 Anders Lövgren, Nowise Systems
 * ----------------------------------------------------------------------
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * ----------------------------------------------------------------------
 *  Contact: Anders Lövgren <andlov@nowise.se>
 * ----------------------------------------------------------------------
 */

#ifndef __CGPSDDOS_H__
#define __CGPSDDOS_H__

#include <stdio.h>
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#define CGPS_NO_EXTERN_PROTOTYPE

#include "dllist.h"

#define CGPSDDOS_RESOLVE_RETRIES 5
#define CGPSDDOS_RESOLVE_TIMEOUT 2

#define CGPSDDOS_MASTER 1
#define CGPSDDOS_SLAVE  2
#define CGPSDDOS_LOCAL  3

#define CGPSDDOS_BUFF_LEN 512    /* max size of master <-> slave messages */

#define CGPSDDOS_MASTER_PORT CGPSD_DEFAULT_PORT + 1
#define CGPSDDOS_SLAVE_PORT  CGPSD_DEFAULT_PORT + 2

#define CGPSDDOS_STATE_INIT 0
#define CGPSDDOS_STATE_QUIT 1
#define CGPSDDOS_STATE_LOOP 2
#define CGPSDDOS_STATE_BUSY 4
#define CGPSDDOS_STATE_FREE 8

/*
 * protocol stage:
 */
#define CGPSDDOS_PROTO_INIT   0  /* initilized */
#define CGPSDDOS_PROTO_FORMAT 1  /* format received */
#define CGPSDDOS_PROTO_DATA   2  /* data received */
#define CGPSDDOS_PROTO_START  3  /* start command received */
#define CGPSDDOS_PROTO_RESULT 4  /* result received */

#define CGPSDDOS_PEER_TIMEOUT (15 * 60)

#define CGPSDDOS_THREAD_STACKSIZE  64 * 1024     /* thread minimum stacksize */
#define CGPSDDOS_THREAD_WRSLEEP    1000000       /* wait before retry (microseconds) */
#define CGPSDDOS_THREAD_WRLIMIT    100           /* retry limit before giving up */
#define CGPSDDOS_THREAD_FILES_MAX  16 * 1024     /* maximum number of open files (only root) */
#define CGPSDDOS_THREAD_SPAWN_MIN  200           /* minimum number of running threads */
#define CGPSDDOS_THREAD_SPAWN_MAX  10000         /* maximum number of started threads (report) */

#define CGPSDDOS_ERRNO_MAX 256   /* guessing largest errno */

#define cgpsddos_quit(state) ((state) & CGPSDDOS_STATE_QUIT)

struct cgpsddos
{
	struct options *opts;
	char *accept;            /* accept connections from this host */
	char *slaves;            /* file with list of slaves */
	int mode;                /* master, slave or local */
	int family;              /* address family (IPv4 or IPV6) */
	int timeout;             /* timeout waiting for peer (seconds) */
};

struct cgpspeer
{
	struct addrinfo *ainf;  /* address info */
	char *addr;             /* host address (supplied) */
	char *port;             /* service port (supplied) */
	char *host;             /* hostname (resolved) */
	char *serv;             /* service name (resolved) */
	int stage;              /* protocol stage */
};

struct resolve_data
{
	struct cgpsddos *ddos;
	struct cgpspeer *peer;
};

struct resolve_slave
{
	struct resolve_data *data;
	pthread_t thread;
};

/*
 * Run in master, slave or local mode.
 */
void run_master(struct cgpsddos *ddos);
void run_slave(struct cgpsddos *ddos);
void run_local(struct cgpsddos *ddos);

/*
 * Read command line options.
 */
void parse_options(int argc, char **argv, struct cgpsddos *ddos);

/*
 * Create a bind (named) TCP or UDP socket (family). The family is either 
 * AF_INET or AF_INET6 or AF_UNSPEC (any IPv4 or IPv6). The type argument 
 * is either SOCK_STREAM or SOCK_DGRAM. See getaddrinfo(3) for information
 * about the addr, port and flags argument.
 */
int open_named_socket(int family, int type, const char *addr, const char *port, int flags);

/*
 * Send datagram message to (unconnected) socket sock. The destination host is
 * defined by the sockaddr argument or should be NULL if socket is connected.
 */
int send_dgram(int sock, const char *buff, ssize_t size, const struct sockaddr *sockaddr, socklen_t addrlen);

/*
 * Receive datagram message from (unconnected) socket. The peer address is 
 * stored in the sockaddr argument of length socklen or should be NULL if 
 * the socket is connected.
 */
int receive_dgram(int sock, char *buff, ssize_t size, struct sockaddr *sockaddr, socklen_t *addrlen);

/*
 * Split the address string into host/port components. The addr argument
 * gets modified by calling this function. The addr uses ':' as separator.
 */
char * split_hostaddr(char *addr, char **host, char **port);

/*
 * Resolve hostname. This function returns a void pointer to an addrinfo 
 * structure filled with info about resolved hostname. The args argument 
 * should point to struct resolve_data data that is filled with reverse 
 * lookup hostname and service. Call freeaddrinfo() on return value
 * to free its allocated memory.
 */
void * resolve_host(void *args);

/*
 * Resolve all slaves in list read from file stream fs. Run each resolve in
 * its own thread to speedup the resolve process for lame DNS servers. When 
 * the function returns, all resolved hosts are inserted in hosts list.
 */
void resolve_slaves(struct cgpsddos *ddos, struct dllist *hosts, FILE *fs);

/*
 * Setup signal handling.
 */
void setup_signals(struct options *opts);

/*
 * Restore signal handling.
 */
void restore_signals(struct options *opts);

#endif /* __CGPSDDOS_H__ */
