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
#include <sys/time.h>
#include <sys/resource.h>
#include <libgen.h>
#include <chemgps.h>

#include "cgpsddos.h"
#include "cgpssqp.h"
#include "cgpsclt.h"

pthread_mutex_t countlock;
pthread_mutex_t finishlock;
pthread_mutex_t failedlock;

pthread_cond_t  countcond;

static unsigned int count = 0;          /* currently number of running threads */
static unsigned int failed = 0;         /* counter for failed predictions */
static unsigned int maxthr = 0;         /* maximum number of running threads */
static unsigned int minthr = 0;         /* minimum number of running threads */
static unsigned int finish = 0;         /* total finished predictions */

int cgpsddos_run(int sock, const struct sockaddr *addr, socklen_t addrlen, struct options *args);

static __inline__ void cgpsddos_print_stat(struct options *popt, int cnt, int min, int max, int fin, int lim)
{
	if(!popt->quiet || popt->verbose || popt->debug) {
		loginfo("running: %d, limit: %d/%d (min/max), started: %d (of %d total)",
			cnt, min, max, fin, lim);
	}
}

static void * cgpsddos_connect(void *args)
{	
	struct client peer;
	struct options mopt = *(struct options *)args;
	int res;

	if(!mopt.quiet) {
		debug("connecting to %s:%d", mopt.ipaddr, mopt.port);
	}

	while(1) {
		if((res = init_socket(&mopt)) == 0) {
			
			peer.type = CGPS_DDOS;
			peer.sock = mopt.unsock ? mopt.unsock : mopt.ipsock;
			peer.opts = &mopt;
	
			res = request(&mopt, &peer);
			close(peer.sock);
		} 
		if(res == 0) {
			pthread_mutex_lock(&finishlock);
			++finish;
			pthread_mutex_unlock(&finishlock);
		} else {
			pthread_mutex_lock(&failedlock);
			++failed;
			pthread_mutex_unlock(&failedlock);
			
			usleep(CGPSDDOS_THREAD_WRSLEEP);
		} 
		pthread_mutex_lock(&finishlock);
		if(finish >= mopt.count) {
			pthread_mutex_unlock(&finishlock);
			break;
		}
		pthread_mutex_unlock(&finishlock);
	}

	pthread_mutex_lock(&countlock);
	--count;
	pthread_mutex_unlock(&countlock);
	pthread_cond_signal(&countcond);
	
	pthread_exit(NULL);
}

int cgpsddos_run(int sock, const struct sockaddr *addr, socklen_t addrlen, struct options *args)
{
	struct timeval ts;      /* predict start */
	struct timeval te;      /* predict finished */
	char msg[CGPSDDOS_BUFF_LEN];	
	pthread_t *threads;
	pthread_attr_t attr;
	/* unsigned int thrmax = 0, thrnow = 0, thrrep = CGPSDDOS_THREAD_SPAWN_MAX; */
	size_t stacksize;
	struct rlimit rlim;
	unsigned int i;
	
	debug("allocating %d threads pool", args->count);
	threads = malloc(sizeof(pthread_t) * args->count);
	if(!threads) {	
		die("failed alloc memory");
	}
	memset(threads, 0, args->count * sizeof(pthread_t));
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_attr_getstacksize(&attr, &stacksize);
	debug("default stack size: %d kB", stacksize / 1024);	
	stacksize = CGPSDDOS_THREAD_STACKSIZE;
	debug("wanted stack size: %d kB", stacksize / 1024);	
	pthread_attr_setstacksize(&attr, stacksize);
	pthread_attr_getstacksize(&attr, &stacksize);
	debug("actual stack size: %d kB", stacksize / 1024);
	
	if(getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
		debug("default maximum file descriptor number: %d (soft), %d (hard)", 
		      rlim.rlim_cur, rlim.rlim_max);
		if(args->count > rlim.rlim_cur) {
			if(rlim.rlim_cur < rlim.rlim_max) {
				if(getuid() == 0) {
					if(rlim.rlim_cur < CGPSDDOS_THREAD_FILES_MAX) {
						rlim.rlim_cur = CGPSDDOS_THREAD_FILES_MAX;
					}
					rlim.rlim_max = rlim.rlim_cur;
				} else {
					rlim.rlim_cur = rlim.rlim_max;
				}
				debug("wanted maximum file descriptor number: %d (soft), %d (hard)", 
				      rlim.rlim_cur, rlim.rlim_max);
				if(setrlimit(RLIMIT_NOFILE, &rlim) < 0) {
					logerr("failed set maximum file descriptor number: %d", rlim.rlim_cur);
				} else {
					getrlimit(RLIMIT_NOFILE, &rlim);
					debug("actual maximum file descriptor number: %d (soft), %d (hard)", 
					      rlim.rlim_cur, rlim.rlim_max);
				}
			}
		} 
	}
	debug("maximum number of open files: %d (from sysconf)", sysconf(_SC_OPEN_MAX));
	
	maxthr = sysconf(_SC_OPEN_MAX) - 1;
	minthr = CGPSDDOS_THREAD_SPAWN_MIN;	
	if(maxthr > args->count) {
		maxthr = args->count;
	}
	if(minthr > maxthr) {
		minthr = maxthr / 2;
	}	
	debug("running threads: %d (max), %d (min)", maxthr, minthr);
	
 	pthread_cond_init(&countcond, NULL);
	pthread_mutex_init(&countlock, NULL);
	pthread_mutex_init(&finishlock, NULL);
	pthread_mutex_init(&failedlock, NULL);
	
	if(gettimeofday(&ts, NULL) < 0) {
		logerr("failed calling gettimeofday()");
		return -1;
	}
	
	loginfo("start running %d predictions", args->count);
	cgpsddos_print_stat(opts, count, minthr, maxthr, finish, args->count);

	pthread_mutex_lock(&countlock);
	for(i = 0; i < maxthr; ++i) {
		if(pthread_create(&threads[i], &attr, cgpsddos_connect, args) == 0) {
			++count;
		}
		debug("thread 0x%lu: started", threads[i]);
	}
	pthread_mutex_unlock(&countlock);
	
        for(;;) {
		pthread_mutex_lock(&countlock);
		debug("waiting for threads to finish (%d running)...", count);
		while(count > 0) {
			if(pthread_cond_wait(&countcond, &countlock) != 0) {
				pthread_mutex_unlock(&countlock);
				continue;
			}
		}
		pthread_mutex_unlock(&countlock);
		debug("all threads has finished");
		break;
	}
	
	cgpsddos_print_stat(opts, count, minthr, maxthr, finish, args->count);
	loginfo("finished running predictions");
	
	if(gettimeofday(&te, NULL) < 0) {
		logerr("failed calling gettimeofday()");
		return -1;
	}

	pthread_mutex_destroy(&failedlock);
	pthread_mutex_destroy(&finishlock);
	pthread_mutex_destroy(&countlock);
 	pthread_cond_destroy(&countcond);
	
	free(threads);
	
	debug("sending predict result to peer");
	snprintf(msg, sizeof(msg), "predict: start=%lu.%lu finish=%lu.%lu thrmax=%d finished=%d failed=%d\n", 
		 ts.tv_sec, ts.tv_usec,
		 te.tv_sec, te.tv_usec, maxthr, finish, failed);
	if(send_dgram(sock, msg, strlen(msg), addr, addrlen) < 0) {
		logerr("failed send to %s", "<fix me: unknown peer>");
	}
	
	return 0;
}
