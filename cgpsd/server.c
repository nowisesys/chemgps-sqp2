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
	errno = 0;
	
	fprintf(peer, "error: %s\n", msg);
	fclose(peer);
	
	debug("sent error '%s' to peer", msg);
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
void service(struct options *popt)
{
	struct workers workers;	
	struct cgps_project proj;	
	fd_set sockfds;  /* server socket file descriptors set */
	int maxsock = 0;
	
        popt->cgps->logger = cgps_syslog;
	popt->cgps->indata = cgps_predict_data;
	
	if(popt->debug > 1) {
		debug("enable library debug");
		popt->cgps->debug = popt->debug;
		popt->cgps->verbose = popt->verbose;
	}
	popt->cgps->syslog = popt->syslog;
	popt->cgps->batch  = 1;
	
	if(cgps_project_load(&proj, popt->proj, popt->cgps) == 0) {
                debug("successful loaded project %s", popt->proj);
		debug("project got %d models", proj.models);
	} else {
		die("failed load project %s", popt->proj);
	}
	
	if(!popt->quiet && popt->verbose) {
		if(popt->ipaddr) {
			loginfo("listening on TCP socket %s port %d", popt->ipaddr, popt->port);
		}
		if(popt->unaddr) {
			loginfo("listening on UNIX socket %s", popt->unaddr);
		}
	}
	
	FD_ZERO(&sockfds);
	if(popt->ipsock) {
		debug("adding TCP server socket to read ready set (fd = %d)", popt->ipsock);
		FD_SET(popt->ipsock, &sockfds);
		maxsock = popt->ipsock > maxsock ? popt->ipsock : maxsock;
	}
	if(popt->unsock) {
		debug("adding UNIX server socket to read ready set (fd = %d)", popt->unsock);
		FD_SET(popt->unsock, &sockfds);
		maxsock = popt->unsock > maxsock ? popt->unsock : maxsock;
	}

	debug("initilizing worker threads...");
	memset(&workers, 0, sizeof(struct workers));
	worker_init(&workers, NULL, process_request);
	
        setup_signals(opts);
	
	debug("enter running state");
        popt->state |= CGPSD_STATE_RUNNING;
	
	if(popt->ipsock && popt->unsock) {
		loginfo("daemon ready (tcp: %s:%d, unix: %s)", popt->ipaddr, popt->port, popt->unaddr);
	} else if(popt->ipsock) {
		loginfo("daemon ready (tcp: %s:%d)", popt->ipaddr, popt->port);
	} else if(popt->unsock) {
		loginfo("daemon ready (unix: %s)", popt->unaddr);
	}
	
	while(1) {
		fd_set readfds = sockfds;
		int result, client = -1;

		debug("waiting for client connections...");
		result = select(maxsock + 1, &readfds, NULL, NULL, NULL);
		
		if(cgpsd_done(popt->state)) {
			break;
		}
		
		if(result < 0) {
			logerr("failed select");
			if(errno == EMFILE || errno == ENFILE) {
				sleep_wait_queue(&workers);
			}
		} else {
			debug("select returned with result = %d", result);
			if(FD_ISSET(popt->ipsock, &readfds)) {
				struct sockaddr_in sockaddr;
				socklen_t socklen = sizeof(struct sockaddr_in);
				client = accept(popt->ipsock,
						(struct sockaddr *)&sockaddr,
						&socklen);
				if(client < 0) {
					logerr("failed accept TCP client connection");
					if(errno == EMFILE || errno == ENFILE) {
						sleep_wait_queue(&workers);
					}
				} else if(!popt->quiet) {
#ifdef HAVE_INET_NTOA
					loginfo("accepted TCP client connection from %s on port %d", 
						inet_ntoa(sockaddr.sin_addr), 
						ntohs(sockaddr.sin_port));
#else
					loginfo("accepted TCP client connection on interface %s and port %d", 
						popt->ipaddr, popt->port);
#endif /* ! HAVE_INET_NTOA */
				}
			}
			if(FD_ISSET(popt->unsock, &readfds)) {
				struct sockaddr_un sockaddr;
				socklen_t socklen = sizeof(struct sockaddr_un);
				client = accept(popt->unsock,
						(struct sockaddr *)&sockaddr,
						&socklen);
				if(client < 0) {
					logerr("failed accept UNIX client connection");
					if(errno == EMFILE || errno == ENFILE) {
						sleep_wait_queue(&workers);
					}
				} else {
					struct ucred cred;
					size_t credlen = sizeof(struct ucred);
					
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
								continue;
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
