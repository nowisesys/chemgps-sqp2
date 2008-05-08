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

#ifdef _GNU_SOURCE
# undef _GNU_SOURCE
#endif
#define _GNU_SOURCE 1

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>
#endif

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

#include "cgpssqp.h"
#include "cgpsd.h"
#include "dllist.h"
#include "worker.h"

/*
 * This function cleanup after the peer has been served.
 */
static void cleanup_request(struct workers *threads, struct client **peer, char **buff)
{	
	if(*peer) {
		int avail;
		
		if((*peer)->ss) {
			fclose((*peer)->ss);
			debug("closed socket stream");
		}
#if defined(USE_IOCTL_SIOCINQ)
		if(ioctl((*peer)->sock, SIOCINQ, &avail) < 0) {
			logerr("failed SIOCINQ ioctl() call on socket");
			avail = 0;
		}
#elif defined(USE_FCNTL_FIONREAD)
		if(fcntl((*peer)->sock, FIONREAD, &avail) < 0) {
			logerr("failed FIONREAD fcntl() call on socket");
			avail = 0;
		}
#endif		
		if(avail) {
			debug("discard remaining bytes on socket");
			while(avail > 0) {
				int ch;
				if(read((*peer)->sock, &ch, 1) < 0) {
					logerr("failed discard available bytes from socket");
					avail = 0;
				}
			}
		}
		if(close((*peer)->sock) < 0) {
			logerr("failed close peer socket");
		} else {
			debug("closed peer socket");
		}
		free(*peer);
		*peer = NULL;
	}
		
	if(buff) {
		free(*buff);
		*buff = NULL;
	}
	
	if(threads) {
		worker_release(threads);
	}
	
	debug("worker thread cleaned up");
}

/*
 * These macros should only be used in inner loop. It sends an optional 
 * protocol error message to peer and cleanup from peer request. Then 
 * it either process next peer (contimue) or quit inner loop (break).
 */
#define process_next_peer(threads, peer) { \
	cleanup_request((threads), &(peer), NULL); \
	if(worker_waiting((threads))) { \
		continue; \
	} \
	break; \
}

#define process_close_peer(threads, peer, msg) { \
	if(msg) { \
		if(fprintf((peer)->ss, "error: %s", (msg)) > 0) { \
			fflush((peer)->ss); \
		} \
	} \
	process_next_peer(threads, peer); \
}

/*
 * Process a peer request. The params argument should have been allocated on
 * the heap and point to a client struct. The void * argument is required as
 * this function is the start routine for one thread.
 */
void * process_request(void *param)
{
	struct workers *threads = (struct workers *)param;
	struct client *peer = NULL;	
	char *buff = NULL;
	size_t size = 0;

	while(1) {
		pthread_mutex_lock(&threads->peerlock);
		debug("locked peer mutex");
		while(!cgpsd_done(opts->state)) {
			debug("blocking thread");
			if(pthread_cond_wait(&threads->peercond, &threads->peerlock) != 0) {
				logerr("failed wait on thread condition");
				pthread_mutex_unlock(&threads->peerlock);
				continue;  /* restart */
			} 
			debug("thread condition wait returned");
			break;
		}
		if(cgpsd_done(opts->state)) {
			debug("exiting worker loop");
			pthread_mutex_unlock(&threads->peerlock);
			break;
		}
		pthread_mutex_unlock(&threads->peerlock);
		debug("unlocked peer mutex");
		
		while((peer = worker_dequeue(threads)) != NULL) {
			struct request_option req;
			struct cgps_options cgps;
			struct cgps_project proj;
			struct cgps_predict pred;
			struct cgps_result res;
			int model, i;

			debug("dequeued socket %d", peer->sock);

			peer->ss = fdopen(dup(peer->sock), "r+");
			if(!peer->ss) {
				logerr("failed open socket stream");
				process_next_peer(threads, peer);
			}
			errno = 0;
			debug("opened socket stream");
			
			debug("sending greeting");
			if(fprintf(peer->ss, "CGPSP %s (%s: server ready)\n", CGPSP_PROTO_VERSION, opts->prog) > 0) {
				fflush(peer->ss);
			}
			if(errno == EPIPE) {
				logerr("socket closed by peer");
				process_next_peer(threads, peer);
			} 
			
			debug("reading greeting");
			if(read_request(&buff, &size, peer->ss) < 0) {
				process_next_peer(threads, peer);
			}
			debug("received: '%s'", buff);

			debug("copying global libchemgps options");
			cgps = *peer->opts->cgps;
			debug("copying project");
			proj = *peer->proj;
			proj.opts = &cgps;
						
			debug("receiving predict request");
			if(read_request(&buff, &size, peer->ss) < 0) {
				process_next_peer(threads, peer);
			}
			debug("received: '%s'", buff);
			if(split_request_option(buff, &req) == CGPSP_PROTO_LAST) {
				logerr("failed read client option (%s)", buff);
				process_close_peer(threads, peer, "unknown option");
			}
			if(req.symbol != CGPSP_PROTO_PREDICT) {
				logerr("protocol error (expected predict option, got %s)", req.option);
				process_close_peer(threads, peer, "expected predict");
			}
			cgps.result = cgps_get_predict_mask(req.value);

			debug("receiving format request");
			if(read_request(&buff, &size, peer->ss) < 0) {
				process_next_peer(threads, peer);
			}
			debug("received: '%s'", buff);
			if(split_request_option(buff, &req) == CGPSP_PROTO_LAST) {
				logerr("failed read client option (%s)", buff);
				process_close_peer(threads, peer, "unknown option");
			}
			if(req.symbol != CGPSP_PROTO_FORMAT) {
				logerr("protocol error (expected format option, got %s)", req.option);
				process_close_peer(threads, peer, "expected format");
			}
			if(strcmp("plain", req.value) == 0) {
				cgps.format = CGPS_OUTPUT_FORMAT_PLAIN;
			} else if(strcmp("xml", req.value) == 0) {
				cgps.format = CGPS_OUTPUT_FORMAT_XML;
			} else {
				logerr("protocol error (invalid format argument, got %s)", req.value);
				process_close_peer(threads, peer, "invalid format");
			}
			
			for(i = 1; i <= proj.models; ++i) {	
				pthread_mutex_lock(&threads->predlock);
				debug("locked mutex for prediction");
				cgps_predict_init(&proj, &pred, peer);
				pthread_mutex_unlock(&threads->predlock);
				debug("unlocked mutex for prediction");
				debug("initilized for prediction");
				pthread_mutex_lock(&threads->predlock);
				debug("locked mutex for prediction");
				if((model = cgps_predict(&proj, i, &pred)) != -1) {
					pthread_mutex_unlock(&threads->predlock);
					debug("unlocked mutex for prediction");
					debug("predict called (index=%d, model=%d)", i, model);
					if(cgps_result_init(&proj, &res) == 0) {
						debug("intilized prediction result");
						if(fprintf(peer->ss, "Result:\n") > 0) {
							fflush(peer->ss);
						}
						if(errno == EPIPE) {
							logerr("socket closed by peer");
							cgps_result_cleanup(&proj, &res);
							pthread_mutex_lock(&threads->predlock);
							cgps_predict_cleanup(&proj, &pred);
							pthread_mutex_unlock(&threads->predlock);
							break;
						}
						pthread_mutex_lock(&threads->predlock);
						debug("locked mutex for prediction");
						if(cgps_result(&proj, model, &pred, &res, peer->ss) == 0) {
							debug("successful got result");
						}
						pthread_mutex_unlock(&threads->predlock);
						debug("unlocked mutex for prediction");
						fflush(peer->ss);
						debug("cleaning up the result");
						cgps_result_cleanup(&proj, &res);
					}
				}
				else {
					pthread_mutex_unlock(&threads->predlock);
					debug("unlocked mutex for prediction");
					logerr("failed predict");
				}
				debug("cleaning up after predict");
				pthread_mutex_lock(&threads->predlock);
				debug("locked mutex for prediction");
				cgps_predict_cleanup(&proj, &pred);
				pthread_mutex_unlock(&threads->predlock);
				debug("unlocked mutex for prediction");
			}
			
			cleanup_request(threads, &peer, NULL);
			if(!worker_waiting(threads)) {
				break;
			}
		}
	}
	debug("cleaning up thread resources");
	cleanup_request(NULL, &peer, &buff);
	
	debug("calling pthread_exit()");
	pthread_exit(NULL);
}
