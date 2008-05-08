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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>
#endif
#include <sys/un.h>
#include <pwd.h>

#include "cgpssqp.h"
#include "cgpsd.h"
#include "dllist.h"
#include "worker.h"

#if !defined(HAVE_PTHREAD_YIELD) && defined(HAVE_SCHED_YIELD)
# if defined(HAVE_SCHED_H)
#  include <sched.h>
# endif
#endif

#ifdef HAVE_LIBPTHREAD
# if ! defined(HAVE_PTHREAD_YIELD) || defined(NEED_PTHREAD_YIELD_DECL)
extern int pthread_yield (void) __THROW;
# endif
#endif

static int thread_yield(void)
{
#if defined(HAVE_PTHREAD_YIELD)
	return pthread_yield();
#elif defined(HAVE_SCHED_YIELD)
	return sched_yield();
#endif
	return 0;
}

/*
 * Send error message to peer and close socket.
 */
static void send_error(int sock, const char *msg)
{
	FILE *peer = fdopen(sock, "r+");
	fprintf(peer, "error: %s", msg);
	fclose(peer);
}

/*
 * Sleep until number of ready peers in wait queue has shrinked.
 */
static void sleep_wait_queue(struct workers *threads)
{
	int count, limit;
	
	count = worker_waiting(threads);
	limit = count > threads->wlimit ? count - threads->wlimit : 0;
	
	logwarn("too many queued peers (%d), sleeping waiting for %d peers to finish", count, threads->wlimit);
	while(count > limit) {
		debug("sleeping waiting for %d peers to finish", count - limit);
		thread_yield();
		usleep(threads->wsleep);
		count = worker_waiting(threads);
	}
}

/*
 * Start accepting connections on server socket(s).
 */
void service(struct options *opts)
{
	struct workers workers;	
	struct cgps_project proj;	
	fd_set sockfds;  /* server socket file descriptors set */
	int maxsock = 0;
	
        opts->cgps->logger = cgps_syslog;
	opts->cgps->indata = cgps_predict_data;
	
	if(opts->debug > 1) {
		debug("enable library debug");
		opts->cgps->debug = opts->debug;
		opts->cgps->verbose = opts->verbose;
	}
	opts->cgps->syslog = opts->syslog;
	opts->cgps->batch  = 1;
	
	if(cgps_project_load(&proj, opts->proj, opts->cgps) == 0) {
                debug("successful loaded project %s", opts->proj);
		debug("project got %d models", proj.models);
	} else {
		die("failed load project %s", opts->proj);
	}
	
	if(!opts->quiet) {
		if(opts->ipaddr) {
			loginfo("listening on TCP socket %s port %d", opts->ipaddr, opts->port);
		}
		if(opts->unaddr) {
			loginfo("listening on UNIX socket %s", opts->unaddr);
		}
	}
	
	FD_ZERO(&sockfds);
	if(opts->ipsock) {
		debug("adding TCP server socket to read ready set (fd = %d)", opts->ipsock);
		FD_SET(opts->ipsock, &sockfds);
		maxsock = opts->ipsock > maxsock ? opts->ipsock : maxsock;
	}
	if(opts->unsock) {
		debug("adding UNIX server socket to read ready set (fd = %d)", opts->unsock);
		FD_SET(opts->unsock, &sockfds);
		maxsock = opts->unsock > maxsock ? opts->unsock : maxsock;
	}

	debug("initilizing worker threads...");
	memset(&workers, 0, sizeof(struct workers));
	worker_init(&workers, NULL, process_request);
	
        setup_signals(opts);
	
	debug("enter running state");
        opts->state |= CGPSD_STATE_RUNNING;
	
	if(opts->ipsock && opts->unsock) {
		loginfo("daemon ready (tcp: %s:%d, unix: %s)", opts->ipaddr, opts->port, opts->unaddr);
	} else if(opts->ipsock) {
		loginfo("daemon ready (tcp: %s:%d)", opts->ipaddr, opts->port);
	} else if(opts->unsock) {
		loginfo("daemon ready (unix: %s)", opts->unaddr);
	}
	
	while(1) {
		fd_set readfds = sockfds;
		int result, client = -1;

		debug("waiting for client connections...");
		result = select(maxsock + 1, &readfds, NULL, NULL, NULL);
		
		if(cgpsd_done(opts->state)) {
			break;
		}
		
		if(result < 0) {
			logerr("failed select");
			if(errno == EMFILE || errno == ENFILE) {
				sleep_wait_queue(&workers);
			}
		} else {
			debug("select returned with result = %d", result);
			if(FD_ISSET(opts->ipsock, &readfds)) {
				struct sockaddr_in sockaddr;
				socklen_t socklen = sizeof(struct sockaddr_in);
				client = accept(opts->ipsock,
						(struct sockaddr *)&sockaddr,
						&socklen);
				if(client < 0) {
					logerr("failed accept TCP client connection");
					if(errno == EMFILE || errno == ENFILE) {
						sleep_wait_queue(&workers);
					}
				} else if(!opts->quiet) {
#ifdef HAVE_INET_NTOA
					loginfo("accepted TCP client connection from %s on port %d", 
						inet_ntoa(sockaddr.sin_addr), 
						ntohs(sockaddr.sin_port));
#else
					loginfo("accepted TCP client connection on interface %s and port %d", 
						opts->ipaddr, opts->port);
#endif /* ! HAVE_INET_NTOA */
				}
			}
			if(FD_ISSET(opts->unsock, &readfds)) {
				struct sockaddr_un sockaddr;
				socklen_t socklen = sizeof(struct sockaddr_un);
				client = accept(opts->unsock,
						(struct sockaddr *)&sockaddr,
						&socklen);
				if(client < 0) {
					logerr("failed accept UNIX client connection");
					if(errno == EMFILE || errno == ENFILE) {
						sleep_wait_queue(&workers);
					}
				} else {
					struct ucred cred;
					int credlen = sizeof(struct ucred);
					
					if(getsockopt(client, SOL_SOCKET, SO_PEERCRED, &cred, &credlen) < 0) {
						logerr("failed get credentials of UNIX socket peer");
					} else {
						debug("UNIX socket peer: pid = %d, uid = %d, gid = %d",
						      cred.pid, cred.uid, cred.gid);
						while(1) {
							struct passwd *pwent = getpwuid(cred.uid);
							if(pwent) {
								loginfo("accepted UNIX client connection from %s (uid: %d, pid: %d)", 
									pwent->pw_name, cred.uid, cred.pid);
								break;
							} else if(errno == EMFILE || errno == ENFILE) {
								sleep_wait_queue(&workers);
							} else {
								send_error(client, "internal server error");
								logerr("failed get password database record for uid=%d", cred.uid);
								break;
							}
						}
					}
				}
			}
			
			if(client != -1) {
				if(worker_enqueue(&workers, client, opts, &proj) < 0) {
					send_error(client, "server busy");
					logerr("failed enqueue peer");
				}
			}
		}
	}
	debug("the done flag is set, exiting service()");
        restore_signals(opts);

	debug("finish worker threads...");
	worker_cleanup(&workers);
	
	debug("closing project");
	cgps_project_close(&proj);	
}
