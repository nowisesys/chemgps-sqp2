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
pthread_mutex_t blocklock;
pthread_mutex_t droplock;

pthread_mutex_t finishlock;
pthread_mutex_t failedlock;
pthread_mutex_t runninglock;

pthread_cond_t  countcond;
pthread_cond_t  blockcond;

static unsigned int count = 0;          /* currently number of running threads */
static unsigned int blocked = 0;        /* currently number of blocked threads */
static unsigned int dropped = 0;        /* currently number of dropped threads */

static unsigned int failed = 0;         /* counter for failed predictions */
static unsigned int finish = 0;         /* total finished predictions */
static unsigned int running = 0;        /* currently number of running predictions */

static unsigned int maxthr = 0;         /* maximum number of running threads */
static unsigned int minthr = 0;         /* minimum number of running threads */

static unsigned int errors[CGPSDDOS_ERRNO_MAX];

int cgpsddos_run(int sock, const struct sockaddr *addr, socklen_t addrlen, struct options *args);

static char * cgpsddos_errors(void)
{
	FILE *fs;
	char *buff = NULL;
	size_t size = 0;
	int i, del = 0;
	
	fs = open_memstream(&buff, &size);
	if(fs) {
		for(i = 0; i < CGPSDDOS_ERRNO_MAX; ++i) {
			if(errors[i]) {
				if(del++) {
					fprintf(fs, ", ");
				}
				fprintf(fs, "'%s (%d)'=%d", strerror(i), i, errors[i]);
			}
		}
		fclose(fs);
	} else {
		logerr("failed open memory stream");
	}
	
	return buff;
}

static __inline__ void cgpsddos_print_stats(struct options *popt)
{
	debug("threads: { count=%d, running=%d, blocked=%d, dropped=%d }, requests: { finished=%d, failed=%d, total=%d }",
	      count, running, blocked, dropped, finish, failed, popt->count);
}

static void cgpsddos_send_result(int sock, const struct sockaddr *addr, socklen_t addrlen,
				 struct timeval *ts, struct timeval *te, struct options *args)
{
	char msg[CGPSDDOS_BUFF_LEN];	
	char *errmsg;
	
	errmsg = cgpsddos_errors();	
	snprintf(msg, sizeof(msg), "predict: time: { start=%lu.%lu, finish=%lu.%lu }, threads: { max=%d, min=%d, dropped=%d }, requests: { finished=%d, failed=%d, total=%d }, errors: { %s }\n", 
		 ts->tv_sec, ts->tv_usec,
		 te->tv_sec, te->tv_usec, maxthr, minthr, dropped, finish, failed, args->count, errmsg ? errmsg : "" );
	if(send_dgram(sock, msg, strlen(msg), addr, addrlen) < 0) {
		logerr("failed send to %s", "<fix me: unknown peer>");
	}
	if(errmsg) {
		free(errmsg);
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

	mopt.noretry = 1;
	mopt.quiet = 1;
	
	while(1) {		
		pthread_mutex_lock(&finishlock);
		pthread_mutex_lock(&runninglock);
		if(finish + running >= mopt.count) {
			pthread_mutex_unlock(&runninglock);
			pthread_mutex_unlock(&finishlock);
			break;
		}
		++running;
		pthread_mutex_unlock(&runninglock);
		pthread_mutex_unlock(&finishlock);
		
		cgpsddos_print_stats(&mopt);
		errno = 0;
		
		if((res = init_socket(&mopt)) == CGPSCLT_CONN_SUCCESS) {
			
			peer.type = CGPS_DDOS;
			peer.sock = mopt.unsock ? mopt.unsock : mopt.ipsock;
			peer.opts = &mopt;
	
			res = request(&mopt, &peer);
			close(peer.sock);
		} 
		pthread_mutex_lock(&runninglock);
		--running;
		pthread_mutex_unlock(&runninglock);
		
		if(res == CGPSCLT_CONN_SUCCESS) {
			pthread_mutex_lock(&finishlock);
			++finish;
			pthread_mutex_unlock(&finishlock);
			pthread_cond_signal(&blockcond);
		} 
		else {
			pthread_mutex_lock(&failedlock);
			++failed;
			++errors[errno];
			pthread_mutex_unlock(&failedlock);
			if(errno == EMFILE) {
				pthread_mutex_lock(&droplock);
				++dropped;
				pthread_mutex_unlock(&droplock);
				break;
			}
		}
		if(res != CGPSCLT_CONN_SUCCESS) {
			pthread_mutex_lock(&blocklock);
			++blocked;
			pthread_mutex_unlock(&blocklock);
			while(1) {
				pthread_mutex_lock(&blocklock);
				if(pthread_cond_wait(&blockcond, &blocklock) != 0) {
					pthread_mutex_unlock(&blocklock);
					continue;
				}
				--blocked;
				pthread_mutex_unlock(&blocklock);
				break;
			}
		}
	}
	debug("finished loop");

	pthread_mutex_lock(&countlock);
	--count;
	pthread_mutex_unlock(&countlock);
	pthread_cond_signal(&countcond);
	
	pthread_mutex_lock(&blocklock);
	while(blocked > 0) {
		pthread_mutex_unlock(&blocklock);
		pthread_cond_signal(&blockcond);
	}
	pthread_mutex_unlock(&blocklock);
	
	pthread_exit(NULL);
}

int cgpsddos_run(int sock, const struct sockaddr *addr, socklen_t addrlen, struct options *args)
{
	struct timeval ts;      /* predict start */
	struct timeval te;      /* predict finished */
	pthread_t *threads;
	pthread_attr_t attr;
	size_t stacksize;
	struct rlimit rlim;
	unsigned int i;
	
	count = blocked = failed = finish = running = maxthr = minthr = 0;
	memset(errors, 0, sizeof(errors));
	
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
	
	maxthr = sysconf(_SC_OPEN_MAX);
	minthr = CGPSDDOS_THREAD_SPAWN_MIN;	
	if(maxthr > args->count) {
		maxthr = args->count;
	}
	if(minthr > maxthr) {
		minthr = maxthr / 2;
	}	
	debug("thread pool: %d (max), %d (min)", maxthr, minthr);

	debug("allocating %d threads pool", maxthr);
	threads = malloc(sizeof(pthread_t) * maxthr);
	if(!threads) {	
		die("failed alloc memory");
	}
	memset(threads, 0, maxthr * sizeof(pthread_t));
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_attr_getstacksize(&attr, &stacksize);
	debug("default stack size: %d kB", stacksize / 1024);	
	stacksize = CGPSDDOS_THREAD_STACKSIZE;
	debug("wanted stack size: %d kB", stacksize / 1024);	
	pthread_attr_setstacksize(&attr, stacksize);
	pthread_attr_getstacksize(&attr, &stacksize);
	debug("actual stack size: %d kB", stacksize / 1024);
	
 	pthread_cond_init(&countcond, NULL);
	pthread_cond_init(&blockcond, NULL);
	
	pthread_mutex_init(&countlock, NULL);
	pthread_mutex_init(&blocklock, NULL);
	pthread_mutex_init(&droplock, NULL);
	
	pthread_mutex_init(&finishlock, NULL);
	pthread_mutex_init(&runninglock, NULL);
	pthread_mutex_init(&failedlock, NULL);
	
	if(gettimeofday(&ts, NULL) < 0) {
		logerr("failed calling gettimeofday()");
		return -1;
	}
	
	loginfo("start running %d predictions", args->count);
	for(i = 0; i < maxthr; ++i) {
		if(pthread_create(&threads[i], &attr, cgpsddos_connect, args) == 0) {
			pthread_mutex_lock(&countlock);
			++count;
			pthread_mutex_unlock(&countlock);
		}
		debug("thread 0x%lu: started", threads[i]);
	}
	
        for(;;) {
		pthread_mutex_lock(&countlock);
		while(count > 0) {
			debug("waiting for threads to finish (%d running)...", count);
			if(pthread_cond_wait(&countcond, &countlock) != 0) {
				continue;
			}
		}
		pthread_mutex_unlock(&countlock);
		break;
	}
	debug("all threads has finished");	
	loginfo("finished running predictions");
	
	if(gettimeofday(&te, NULL) < 0) {
		logerr("failed calling gettimeofday()");
		return -1;
	}

	pthread_mutex_destroy(&failedlock);
	pthread_mutex_destroy(&runninglock);
	pthread_mutex_destroy(&finishlock);
	
	pthread_mutex_destroy(&droplock);
	pthread_mutex_destroy(&blocklock);
	pthread_mutex_destroy(&countlock);
	
 	pthread_cond_destroy(&countcond);
 	pthread_cond_destroy(&blockcond);
	
	free(threads);
	
	debug("sending predict result to peer");
	cgpsddos_send_result(sock, addr, addrlen, &ts, &te, args);
	
	return 0;
}
