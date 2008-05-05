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

/*
 * Open connection to ChemGPS daemon (cgpsd) and run denial of service test.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>
#endif
#include <libgen.h>
#include <chemgps.h>

#include "cgpssqp.h"
#define CGPSCLT_EXTERN 1
#include "cgpsclt.h"
#include "cgpsddos.h"

void * cgpsddos_connect(void *args)
{	
	struct client peer;
	struct options topt = *(struct options *)args;
	
	debug("connecting to %s:%d", topt.ipaddr, topt.port);
	if(init_socket(&topt) < 0) {
		pthread_exit(NULL);
	}
	debug("connected to %s:%d", topt.ipaddr, topt.port);
	
	peer.type = CGPS_DDOS;
	peer.sock = topt.unsock ? topt.unsock : topt.ipsock;
	peer.opts = &topt;
	
	request(&topt, &peer);
	
	close(peer.sock);	
	pthread_exit(NULL);
}

int cgpsddos_run(int sock, const struct sockaddr *addr, socklen_t addrlen, struct options *args)
{
	struct timeval ts;      /* predict start */
	struct timeval te;      /* predict finished */
	char msg[CGPSDDOS_BUFF_LEN];	
	pthread_t *threads;
	pthread_attr_t attr;
	int i, runned = 0, remain = 0, thrmax = 0, thrnow = 0;
	
	debug("allocating %d threads pool", args->count);
	threads = malloc(sizeof(pthread_t) * args->count);
	if(!threads) {	
		die("failed alloc memory");
	}
	memset(threads, 0, args->count * sizeof(pthread_t));
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	if(gettimeofday(&ts, NULL) < 0) {
		logerr("failed calling gettimeofday()");
		return -1;
	}
	
	remain = args->count;
	debug("start running predictions");
	while(runned < args->count) {
		debug("%d predictions of %d total left to run", remain, args->count);
		debug("starting prediction threads");
		thrnow = 0;
		for(i = 0; i < remain; ++i, ++thrnow) {
			if(pthread_create(&threads[i], &attr, cgpsddos_connect, args) != 0) {
				logerr("failed create thread (%d created)", i);
				break;
			}
			debug("thread 0x%lu: started", threads[i]);
		}
		if(thrnow > thrmax) {
			thrmax = thrnow;
		}

		debug("joining prediction threads");
		for(i = 0; i < thrnow; ++i) {
			pthread_join(threads[i], NULL);
			debug("thread 0x%lu: joined", threads[i]);
		}
		debug("all prediction threads joined");
		
		runned += thrnow;
		remain -= thrnow;
	}	
	debug("finished running predictions");
	
	if(gettimeofday(&te, NULL) < 0) {
		logerr("failed calling gettimeofday()");
		return -1;
	}

	free(threads);
	
	debug("sending predict result to peer");
	snprintf(msg, sizeof(msg), "predict: %lu.%lu %lu.%lu %d %d\n", 
		 ts.tv_sec, ts.tv_usec,
		 te.tv_sec, te.tv_usec, thrmax, runned);
	if(send_dgram(sock, msg, strlen(msg), addr, addrlen) < 0) {
		logerr("failed send to %s", "<fix me: unknown peer>");
	}
		 
	return 0;
}
