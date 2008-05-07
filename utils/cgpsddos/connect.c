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

#include "cgpssqp.h"
#define CGPSCLT_EXTERN 1
#include "cgpsclt.h"
#include "cgpsddos.h"

pthread_mutex_t countlock;
pthread_cond_t  countcond;
static int count = 0;          /* currently number of running threads */
static int failed = 0;         /* counter for failed threads */

static int maxthr = 0;         /* maximum number of running threads */
static int minthr = 0;         /* minimum number of running threads */

static inline void cgpsddos_print_stat(struct options *opts, int count, int minthr, int maxthr, int finished, int limit)
{
	if(!opts->quiet || opts->verbose) {
		loginfo("running: %d, limit: %d/%d (min/max), started: %d (of %d total)",
			count, minthr, maxthr, finished, limit);
	}
}

static int cgpsddos_init_socket(struct options *topt)
{
	int res, retry = 0;
	
	while((res = init_socket(topt)) != CGPSCLT_CONN_SUCCESS) {
		if(res == CGPSCLT_CONN_RETRY) {
			usleep(CGPSDDOS_THREAD_WRSLEEP * ++retry);
			if(retry > CGPSDDOS_THREAD_WRLIMIT) break;
		} else {
			pthread_mutex_lock(&countlock);
			++failed;
			--count;
			pthread_mutex_unlock(&countlock);			
			pthread_exit(NULL);
		}
	}
	
	if(res == CGPSCLT_CONN_SUCCESS) {
		if(!opts->quiet) {
			debug("connected to %s:%d", topt->ipaddr, topt->port);
		}
		return 0;
	} 
	
	return -1;
}

static int cgpsddos_request(struct options *topt, struct client *peer)
{
	int res, retry = 0;
	
	while((res = request(topt, peer)) == CGPSCLT_CONN_RETRY) {
		usleep(CGPSDDOS_THREAD_WRSLEEP * ++retry);
		if(retry > CGPSDDOS_THREAD_WRLIMIT) break;
	}
	
	if(res == CGPSCLT_CONN_SUCCESS) {
		return 0;
	}

	pthread_mutex_lock(&countlock);
	++failed;
	pthread_mutex_unlock(&countlock);
	
	return -1;
}

void * cgpsddos_connect(void *args)
{	
	struct client peer;
	struct options topt = *(struct options *)args;

	if(!opts->quiet) {
		debug("connecting to %s:%d", topt.ipaddr, topt.port);
	}
	
	if(cgpsddos_init_socket(&topt) == 0) {
		peer.type = CGPS_DDOS;
		peer.sock = topt.unsock ? topt.unsock : topt.ipsock;
		peer.opts = &topt;
	
		cgpsddos_request(&topt, &peer);
		close(peer.sock);
	}
	
	pthread_mutex_lock(&countlock);
	if(--count < minthr) {
		pthread_cond_signal(&countcond);
	}
	pthread_mutex_unlock(&countlock);
	
	pthread_exit(NULL);
}

int cgpsddos_run(int sock, const struct sockaddr *addr, socklen_t addrlen, struct options *args)
{
	struct timeval ts;      /* predict start */
	struct timeval te;      /* predict finished */
	char msg[CGPSDDOS_BUFF_LEN];	
	pthread_t *threads;
	pthread_attr_t attr;
	int finished = 0, thrmax = 0, thrnow = 0, thrrep = CGPSDDOS_THREAD_SPAWN_MAX;
	size_t stacksize;
	struct rlimit rlim;
	
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
	if(minthr > maxthr) {
		minthr = maxthr / 2;
	}
	debug("running threads: %d (max), %d (min)", maxthr, minthr);
	
	if(args->count < thrrep) {
		thrrep = args->count / 4;
	} else {
		if(args->count / thrrep > 1000) {
			thrrep = args->count / 1000;
			while(thrrep % 10000 != 0) --thrrep;
		}
	}
	debug("report thread stats every %d thread started", thrrep);
	
 	pthread_cond_init(&countcond, NULL);
	pthread_mutex_init(&countlock, NULL);
	
	if(gettimeofday(&ts, NULL) < 0) {
		logerr("failed calling gettimeofday()");
		return -1;
	}
	
	debug("start running predictions");
	cgpsddos_print_stat(opts, count, minthr, maxthr, finished, args->count);
	
	while(finished < args->count) {
		debug("%d predictions of %d total left to run", args->count - finished, args->count);
		debug("starting prediction threads");
		thrnow = 0;
		for(; finished < args->count; ++thrnow, ++finished) {
			pthread_mutex_lock(&countlock);
			if(finished % thrrep == 0 && finished != 0) {
				cgpsddos_print_stat(opts, count, minthr, maxthr, finished, args->count);
			}
			if(++count >= maxthr) {
				pthread_mutex_unlock(&countlock);
				break;
			}
			pthread_mutex_unlock(&countlock);
			if(pthread_create(&threads[finished], &attr, cgpsddos_connect, args) != 0) {
				break;
			}
			debug("thread 0x%lu: started", threads[finished]);
		}
		
		if(thrnow > thrmax) {
			thrmax = thrnow;
		}
		
		pthread_mutex_lock(&countlock);
		while(count > minthr) {
			if(pthread_cond_wait(&countcond, &countlock) != 0) {
				pthread_mutex_unlock(&countlock);
				continue;
			}
			break;
		}
		pthread_mutex_unlock(&countlock);
	}
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
	
	debug("finished running predictions");
	cgpsddos_print_stat(opts, count, minthr, maxthr, finished, args->count);
	
	if(gettimeofday(&te, NULL) < 0) {
		logerr("failed calling gettimeofday()");
		return -1;
	}

	pthread_mutex_destroy(&countlock);
 	pthread_cond_destroy(&countcond);
	
	free(threads);
	
	debug("sending predict result to peer");
	snprintf(msg, sizeof(msg), "predict: start=%lu.%lu finish=%lu.%lu thrmax=%d finished=%d failed=%d\n", 
		 ts.tv_sec, ts.tv_usec,
		 te.tv_sec, te.tv_usec, thrmax, finished, failed);
	if(send_dgram(sock, msg, strlen(msg), addr, addrlen) < 0) {
		logerr("failed send to %s", "<fix me: unknown peer>");
	}
	
	return 0;
}
