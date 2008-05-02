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
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#include <poll.h>
#include <sys/sendfile.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#include "cgpssqp.h"
#include "cgpsddos.h"

/*
 * Cleanup slave data.
 */
static void slave_cleanup(void *args)
{
	struct cgpspeer *peer = (struct cgpspeer *)args;
	free(peer->addr);
	free(peer->port);
	free(peer->host);
	free(peer->serv);
	freeaddrinfo(peer->ainf);
	free(peer);
}

/*
 * Compare the address info of two slave nodes.
 */
static int slave_compare(void *data1, void *data2)
{
	struct cgpspeer *peer1 = (struct cgpspeer *)data1;
	struct cgpspeer *peer2 = (struct cgpspeer *)data2;
	return peer1 - peer2;
}

/*
 * Creates a named UDP socket.
 */
static void create_master_socket(struct cgpsddos *ddos)
{
	char port[6];

	if(!ddos->family) {
		ddos->family = AF_UNSPEC;
	}
	if(!ddos->opts->port) {
		ddos->opts->port = CGPSDDOS_MASTER_PORT;
	}
	snprintf(port, sizeof(port), "%d", ddos->opts->port);
	
	debug("creating master socket");
	ddos->opts->ipsock = open_named_socket(ddos->family, SOCK_DGRAM, NULL, port, AI_ADDRCONFIG | AI_PASSIVE);
	if(ddos->opts->ipsock < 0) {
		die("failed create master socket");
	}
	debug("master socket created");
}

/*
 * Try send message.
 */
static int send_message(struct cgpsddos *ddos, struct cgpspeer *peer, const char *msg)
{
	struct addrinfo *next = NULL;

	for(next = peer->ainf; next != NULL; next = next->ai_next) {
		if(send_dgram(ddos->opts->ipsock, 
			      msg, 
			      strlen(msg),
			      next->ai_addr, 
			      next->ai_addrlen) > 0) {
			debug("successful sent message to peer (%s:%s)", peer->addr, peer->port);
			return 0;
		}
		debug("trying next address...");
	}
	
	logerr("failed sent message to peer (%s:%s)", peer->addr, peer->port);
	return -1;
}

/*
 * Try send file.
 */
int send_file(int sock, const struct sockaddr *addr, socklen_t addrlen, const char *path, off_t size)
{
	int fd;
	
	if((fd = open(path, 0, O_RDONLY)) < 0) {
		logerr("failed open %s for reading", path);
		return -1;
	}

	if(connect(sock, addr, addrlen) < 0) {
		logerr("failed connect to peer");
		close(fd);
		return -1;
	}
	if(sendfile(sock, fd, NULL, size) < 0) {
		logerr("failed send file to slave");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

/*
 * The communication with the slaves is done using UDP, so we just send them
 * the start message and wait for them to finish. The only operation that may
 * take long time is resolving addresses, so we run them in its own thread.
 */
void run_master(struct cgpsddos *ddos)
{
	FILE *fs;	
	struct cgpspeer *peer;
	struct dllist slaves;
	char msg[CGPSDDOS_BUFF_LEN];
	
	loginfo("running in master mode");
	create_master_socket(ddos);
	
	if(ddos->slaves) {
		fs = fopen(ddos->slaves, "r");
		if(!fs) {
			die("failed open %s", ddos->slaves);
		}
	}
	else {
		loginfo("reading client list from stdin (hit ctrl+d to quit)");
		fs = stdin;
	}
	
	debug("initilize list of slaves");
	dllist_init(&slaves, slave_cleanup, slave_compare);
	
	debug("begin resolve slave addresses");
	resolve_slaves(ddos, &slaves, fs);
	if(dllist_count(&slaves) == 0) {
		die("no slave addresses resolved");
	}
	debug("finished resolve slave addresses (resolved %d slaves)", dllist_count(&slaves));
	
	if(fs != stdin) {
		fclose(fs);
	}

	debug("sending greeting");
	snprintf(msg, sizeof(msg), "CGPSP %s (%s: master ready)\n", CGPSP_PROTO_VERSION, opts->prog);
	for(peer = dllist_move_first(&slaves); peer; peer = dllist_move_next(&slaves)) {
		if(send_message(ddos, peer, msg) < 0) {
			if(dllist_find_first(&slaves, peer)) {
				logerr("removing failed slave %s:%s", peer->addr, peer->port);
				dllist_remove(&slaves);
			}
		}
	}

	ddos->opts->state = CGPSDDOS_STATE_LOOP;
	while(ddos->opts->state != CGPSDDOS_STATE_QUIT) {
		struct sockaddr_storage sockaddr;
		socklen_t sockaddr_len;
		struct request_option req;
		struct stat st;
		struct pollfd fds;
		nfds_t nfds = 1;
		int res;
		
		fds.events = POLLIN;
		fds.fd = ddos->opts->ipsock;
		
		res = poll(&fds, nfds, ddos->timeout * 1000);
		switch(res) {
		case -1:
			die("failed poll");
			break;
		case 0:
			logerr("no client response within %d seconds, exiting", ddos->timeout);
			ddos->opts->state = CGPSDDOS_STATE_QUIT;
			break;
		default:
			sockaddr_len = sizeof(struct sockaddr_storage);
			if(receive_dgram(ddos->opts->ipsock, 
					 msg, 
					 sizeof(msg), 
					 (struct sockaddr *)&sockaddr, 
					 &sockaddr_len) < 0) { 
				logerr("failed receive from slave");
				continue;
			}
			
			if(ddos->opts->debug > 2) {
				debug("got '%s' from peer", msg);
			}
			
			switch(split_request_option(msg, &req)) {
			case CGPSP_PROTO_GREETING:
				debug("received greeting");
				debug("sending target option (%s)", ddos->opts->ipaddr);
				
				snprintf(msg, sizeof(msg), "target: %s\n", ddos->opts->ipaddr);
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
					      (const struct sockaddr *)&sockaddr, 
					      sizeof(struct sockaddr_storage)) < 0) {
					logerr("failed send to %s", "<fix me: unknown peer>");
				}
				break;
			case CGPSP_PROTO_TARGET:
				debug("received target ack");
				debug("sending result option (%d)", ddos->opts->cgps->result);
				
				snprintf(msg, sizeof(msg), "result: %d\n", ddos->opts->cgps->result);
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
					      (const struct sockaddr *)&sockaddr, 
					      sizeof(struct sockaddr_storage)) < 0) {
					logerr("failed send to %s", "<fix me: unknown peer>");
				}
				break;
			case CGPSP_PROTO_RESULT:
				debug("received result ack");
				debug("sending count option (%d)", ddos->opts->count);
				
				snprintf(msg, sizeof(msg), "count: %d\n", ddos->opts->count);
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
					      (const struct sockaddr *)&sockaddr, 
					      sizeof(struct sockaddr_storage)) < 0) {
					logerr("failed send to %s", "<fix me: unknown peer>");
				}
				break;
			case CGPSP_PROTO_COUNT:
				debug("received count ack");
				debug("sending format option (%d)", ddos->opts->cgps->format);
				
				snprintf(msg, sizeof(msg), "format: %d\n", ddos->opts->cgps->format);
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
					      (const struct sockaddr *)&sockaddr, 
					      sizeof(struct sockaddr_storage)) < 0) {
					logerr("failed send to %s", "<fix me: unknown peer>");
				}
				break;
			case CGPSP_PROTO_FORMAT:
				debug("received format ack");
				debug("sending data (%s)", ddos->opts->data);

				if(stat(ddos->opts->data, &st) < 0) {
					logerr("failed stat %s", ddos->opts->data);
					continue;
				}
				
				snprintf(msg, sizeof(msg), "data: %lu\n", st.st_size);
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
					      (const struct sockaddr *)&sockaddr, 
					      sizeof(struct sockaddr_storage)) < 0) {
					logerr("failed send to %s", "<fix me: unknown peer>");
				}
				
				if(send_file(ddos->opts->ipsock, 
					     (const struct sockaddr *)&sockaddr,
					     sizeof(struct sockaddr_storage),
					     ddos->opts->data, 
					     st.st_size) < 0) {
					logerr("failed send data file to %s", "<fix me: unknown peer>");
				}
				break;
			case CGPSP_PROTO_LOAD:
				debug("received data ack");
				debug("sending start command");
				
				snprintf(msg, sizeof(msg), "start:\n");
				if(send_dgram(ddos->opts->ipsock, msg, strlen(msg), 
					      (const struct sockaddr *)&sockaddr, 
					      sizeof(struct sockaddr_storage)) < 0) {
					logerr("failed send to %s", "<fix me: unknown peer>");
				}
				break;
			case CGPSP_PROTO_START:
				debug("received start command ack");
				loginfo("slave %s has started (waiting for result)", "<fix me: unknown peer>");
				break;
			case CGPSP_PROTO_PREDICT:
				debug("recived predict result");
				loginfo("predict: %s", req.value);
				break;
			case CGPSP_PROTO_ERROR:
				logerr("slave: %s", req.value);
				break;
			case CGPSP_PROTO_LAST:
				logerr("unmatched protocol option (got %s)", req.option);
				break;
			default:
				logerr("protocol error (got option=%s, value=%s, symbol=%d)",
				       req.option, req.value, req.symbol);
				break;
			}
		}
	}
	
	debug("finished run_master()");
}
