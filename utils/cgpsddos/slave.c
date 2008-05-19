/* Simca-QP predictions for the ChemGPS project.
 *
 * Copyright (C) 2007-2008 Anders Lövgren and the Computing Department,
 * Uppsala Biomedical Centre, Uppsala University.
 * 
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
 *  Contact: Anders Lövgren <anders.lovgren@bmc.uu.se>
 * ----------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#include <poll.h>

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#define LINUX_SIOCINQ 1
#if defined(HAVE_LINUX_SOCKIOS_H) && defined(LINUX_SIOCINQ)
# include <linux/sockios.h>    
# define USE_IOCTL_SIOCINQ
#elif defined(HAVE_FCNTL_H)
# include <fcntl.h>
# define USE_FCNTL_FIONREAD
#else
# error "No usable ioctl/fcntl found!"
#endif

#include "cgpsddos.h"
#include "cgpssqp.h"

extern int cgpsddos_run(int sock, const struct sockaddr *addr, socklen_t addrlen, struct options *args);

/*
 * Creates a named UDP socket.
 */
static void create_slave_socket(struct cgpsddos *ddos)
{
	char port[6];
	
        if(!ddos->family) {
		ddos->family = AF_UNSPEC;
	}
	if(!ddos->opts->port) {
		ddos->opts->port = CGPSDDOS_SLAVE_PORT;
	}
	snprintf(port, sizeof(port), "%d", ddos->opts->port);
	
	debug("creating slave socket");
	
	ddos->opts->ipsock = open_named_socket(ddos->family, SOCK_DGRAM, NULL, port, AI_ADDRCONFIG | AI_PASSIVE);
	if(ddos->opts->ipsock < 0) {
		die("failed create slave socket");
	}
	debug("slave socket created");
}

/*
 * Read size data from socket sock into buffer buff. Perhaps this code
 * belongs in receive_dgram() instead.
 */
static int read_data(struct cgpsddos *ddos, int sock, char *buff, off_t size, struct sockaddr *addr, socklen_t *addrlen)
{
	off_t total = 0;
	ssize_t bytes;
	int res, avail;
	struct pollfd fds;
	nfds_t nfds = 1;
	
	while(total < size) {
		avail = 0;
		
		fds.events = POLLIN;
		fds.fd = ddos->opts->ipsock;
		
		res = poll(&fds, nfds, ddos->timeout * 1000);
		if(res < 0) {
			logerr("failed poll peer socket");
			break;
		} else if(res == 0) {
			logerr("timeout waiting for peer data");
			break;
		} else {
#if defined(USE_IOCTL_SIOCINQ)
			if(ioctl(sock, SIOCINQ, &avail) < 0) {
				logerr("failed SIOCINQ ioctl() call on socket");
				break;
			}
#elif defined(USE_FCNTL_FIONREAD)
			if(fcntl(sock, FIONREAD, &avail) < 0) {
				logerr("failed FIONREAD fcntl() call on socket");
				break;
			}
#endif
			if(avail > (size - total)) {
				avail = size - total;
			}
			if(ddos->opts->debug > 1) {
				debug("going to read %d bytes of data from peer", avail);
			}
			bytes = receive_dgram(sock, buff + total, avail, addr, addrlen);
			if(bytes < 0) {
				break;
			}
			total += bytes;
		}
	}
	
	if(total < size) {
		logerr("failed receive data (wanted: %d, got: %d bytes)", size, total);
		return -1;
	}
	
	debug("read %d bytes of data from peer", total);
	return 0;
}

static void cleanup_args(struct options *args)
{
	debug("cleanup options...");
	if(args) {
		if(args->cgps) {
			free(args->cgps);
			args->cgps = NULL;
		}
		if(args->ipaddr) {
			free(args->ipaddr);
			args->ipaddr = NULL;
		}
		if(args->data) {
			free(args->data);
			args->data = NULL;
		}
		free(args);
	}
}

/*
 * We should use dllist to store a new structure (A) for each new master
 * connection created at greeting time. Then we should call cgpsddos_run()
 * in its own thread or child process. Each structure A should contain a
 * unique sequence number associated with that DDOS request.
 */
void run_slave(struct cgpsddos *ddos)
{	
	struct options *args = NULL;
	off_t size;
	
	loginfo("running in slave mode");
	create_slave_socket(ddos);

	ddos->opts->state = CGPSDDOS_STATE_LOOP;
	
	while(!cgpsddos_quit(ddos->opts->state)) {
		struct sockaddr_storage sockaddr;
		socklen_t addrlen;
		struct request_option req;
		char msg[CGPSDDOS_BUFF_LEN];	
		char *addr, *port;
		char host[NI_MAXHOST];
		int res;
		
		debug("waiting for master connections...");
		
		addrlen = sizeof(struct sockaddr_storage);
		if(receive_dgram(ddos->opts->ipsock, msg, sizeof(msg), 
				 (struct sockaddr *)&sockaddr, 
				 &addrlen) < 0) { 
			if(!cgpsddos_quit(ddos->opts->state)) {
				logerr("failed receive from master");
			}
			continue;
		}

		if((res = getnameinfo((const struct sockaddr *)&sockaddr, addrlen,
				      host, sizeof(host), 
				      NULL, 0, NI_NUMERICHOST)) != 0) {
			logerr("failed resolve peer address (%s)", gai_strerror(res));
			snprintf(msg, sizeof(msg), "error: local resolve failure");
			if(send_dgram(ddos->opts->ipsock, msg, strlen(msg),
				      (const struct sockaddr *)&sockaddr, 
				      addrlen) < 0) {
				logerr("failed send to %s", host);
			}
			continue;
		}
		if(ddos->accept && strcmp(ddos->accept, host) != 0) {
			logerr("connection from host %s don't match address filter %s", 
			       host, ddos->accept);
			snprintf(msg, sizeof(msg), "error: you are not allowed");
			if(send_dgram(ddos->opts->ipsock, msg, strlen(msg),
				      (const struct sockaddr *)&sockaddr, 
				      addrlen) < 0) {
				logerr("failed send to %s", host);
			}
			continue;
		}
		
		if(ddos->opts->debug > 2) {
			debug("got '%s' from peer", msg);
		}
		
		switch(split_request_option(msg, &req)) {
		case CGPSP_PROTO_GREETING:
			debug("received greeting");
			if(ddos->opts->state == CGPSDDOS_STATE_BUSY) {
				snprintf(msg, sizeof(msg), "error: request in progress");
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg),
					      (const struct sockaddr *)&sockaddr, 
					      addrlen) < 0) {
					logerr("failed send to %s", host);
				}
			} else {
				args = malloc(sizeof(struct options));
				if(!args) {
					snprintf(msg, sizeof(msg), "error: failed alloc memory");
					if(send_dgram(ddos->opts->ipsock, msg, strlen(msg),
						      (const struct sockaddr *)&sockaddr,
						      addrlen) < 0) {
						logerr("failed send to %s", host);
					}
					die("failed alloc memory");
				}
				memset(args, 0, sizeof(struct options));
	
				args->cgps = malloc(sizeof(struct cgps_options));
				if(!args->cgps) {
					snprintf(msg, sizeof(msg), "error: failed alloc memory");
					if(send_dgram(ddos->opts->ipsock, msg, strlen(msg),
						      (const struct sockaddr *)&sockaddr, 
						      addrlen) < 0) {
						logerr("failed send to %s", host);
					}
					die("failed alloc memory");
				}
				memset(args->cgps, 0, sizeof(struct cgps_options));
				
				args->prog = ddos->opts->prog;
				args->parent = getpid();
				args->debug = ddos->opts->debug;
				args->verbose = ddos->opts->verbose;
				args->quiet = ddos->opts->quiet;

				debug("sending greeting");
				snprintf(msg, sizeof(msg), "CGPSP %s (%s: slave ready)\n", CGPSP_PROTO_VERSION, opts->prog);
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
					      (const struct sockaddr *)&sockaddr, 
					      addrlen) < 0) {
					logerr("failed send to %s", host);
				}
				ddos->opts->state = CGPSDDOS_STATE_BUSY;
			}
			break;
		case CGPSP_PROTO_TARGET:
			debug("received target option");
			
			if(split_hostaddr(req.value, &addr, &port)) {
				args->ipaddr = strdup(addr);
				if(port) {
					args->port = strtoul(port, NULL, 10);
				} else {
					args->port = CGPSD_DEFAULT_PORT;
				}
			}
			if(!args->ipaddr) {
				snprintf(msg, sizeof(msg), "error: failed alloc memory");
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg),
					      (const struct sockaddr *)&sockaddr, 
					      addrlen) < 0) {
					logerr("failed send to %s", host);
				}
				die("failed alloc memory");
			} 
			debug("sending target ack");
			snprintf(msg, sizeof(msg), "target: ok");
			if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
				      (const struct sockaddr *)&sockaddr, 
				      addrlen) < 0) {
				logerr("failed send to %s", host);
			}
			break;
		case CGPSP_PROTO_RESULT:
			debug("received result option");
			
			args->cgps->result = strtoul(req.value, NULL, 10);

			debug("sending result ack");
			snprintf(msg, sizeof(msg), "result: ok");
			if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
				      (const struct sockaddr *)&sockaddr, 
				      addrlen) < 0) {
				logerr("failed send to %s", host);
			}			
			break;
		case CGPSP_PROTO_FORMAT:
			debug("received format option");
			
			args->cgps->format = strtoul(req.value, NULL, 10);
			
			debug("sending format ack");
			snprintf(msg, sizeof(msg), "format: ok");
			if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
				      (const struct sockaddr *)&sockaddr, 
				      addrlen) < 0) {
				logerr("failed send to %s", host);
			}
			break;
		case CGPSP_PROTO_COUNT:
			debug("received count option");
			
			args->count = strtoul(req.value, NULL, 10);
			
			debug("sending count ack");
			snprintf(msg, sizeof(msg), "count: ok");
			if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
				      (const struct sockaddr *)&sockaddr, 
				      addrlen) < 0) {
				logerr("failed send to %s", host);
			}
			break;
		case CGPSP_PROTO_LOAD:
			size = strtoul(req.value, NULL, 10);
			debug("received data option (for %lu bytes)", size);
			
			args->data = malloc(size);
			if(!args->data) {
				snprintf(msg, sizeof(msg), "error: failed alloc memory");
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg),
					      (const struct sockaddr *)&sockaddr,
					      addrlen) < 0) {
					logerr("failed send to %s", host);
				}
				die("failed alloc memory");
			}
			if(read_data(ddos, ddos->opts->ipsock, args->data, size,
				     (struct sockaddr *)&sockaddr, &addrlen) < 0) {
				snprintf(msg, sizeof(msg), "error: failed read %lu bytes of data", size);
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg),
					      (const struct sockaddr *)&sockaddr,
					      addrlen) < 0) {
					logerr("failed send to %s", host);
				}
				logerr("failed read %lu bytes of data", size);
				continue;
			}
			
			debug("sending data ack");
			snprintf(msg, sizeof(msg), "data: ok");
			if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
				      (const struct sockaddr *)&sockaddr, 
				      addrlen) < 0) {
				logerr("failed send to %s", host);
			}
			break;
		case CGPSP_PROTO_START:
			debug("received start command");						
			debug("sending start ack");
			snprintf(msg, sizeof(msg), "start: ok");
			if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
				      (const struct sockaddr *)&sockaddr, 
				      addrlen) < 0) {
				logerr("failed send to %s", host);
			}
			debug("starting cgpsd session");
			if(cgpsddos_run(ddos->opts->ipsock, 
					(const struct sockaddr *)&sockaddr, 
					addrlen, args) < 0) {
				logerr("the cgpsd session failed");
			} else {
				debug("finished cgpsd session");
			}
			cleanup_args(args);
			args = NULL;
			if(!cgpsddos_quit(opts->state)) {
				ddos->opts->state = CGPSDDOS_STATE_FREE;
			}
			break;
		case CGPSP_PROTO_QUIT:
			debug("received quit command");
			debug("sending quit ack");
			snprintf(msg, sizeof(msg), "quit: ok");
			if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
				      (const struct sockaddr *)&sockaddr, 
				      addrlen) < 0) {
				logerr("failed send to %s", host);
			}
			cleanup_args(args);
			args = NULL;
			ddos->opts->state = CGPSDDOS_STATE_QUIT;
			break;
		case CGPSP_PROTO_ERROR:
			logerr("master: %s", req.value);
			break;			
		case CGPSP_PROTO_LAST:
			die("unmatched protocol option (got %s)", req.option);
			break;
		default:
			die("protocol error (got option=%s, value=%s, symbol=%d)", 
			    req.option, req.value, req.symbol);
			break;
		}
	}
	debug("finished run_slave()");
}
