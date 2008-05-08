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

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>
#endif

#include "cgpssqp.h"
#include "dllist.h"
#include "worker.h"

/*
 * Print worker thread pool statistics (for debug).
 */
static void worker_show_stats(const char *where, struct workers *threads)
{
	pthread_mutex_lock(&threads->poollock);
	pthread_mutex_lock(&threads->peerlock);
       
	debug("thread pool status (%s): wait=%d, used=%d, size=%d, grow=%d, mode=%d, max=%d", where,
	      dllist_count(&threads->ready), threads->used, threads->size, 
	      threads->grow, threads->mode, threads->max);
	
	pthread_mutex_unlock(&threads->peerlock);
	pthread_mutex_unlock(&threads->poollock);
}

/*
 * Cleanup ready list.
 */
static void worker_ready_destroy(void *data)
{
	struct client *peer = (struct client *)data;
	if(peer) {
		debug("ready list destroy, closing socket %d", peer->sock);
		if(peer->ss) {
			fclose(peer->ss);
		}
		if(peer->sock != -1) {
			close(peer->sock);
			peer->sock = -1;
		}
		free(peer);
	}
}

/*
 * Compare lookup callback.
 */
static int worker_ready_compare(void *data1, void *data2)
{
	struct client *peer1 = (struct client *)data1;
	struct client *peer2 = (struct client *)data2;
	
	return peer1->sock < peer2->sock;
}

/*
 * Initilize the pool of threads (workers).
 */
int worker_init(struct workers *threads, void *data, void * (*threadfunc)(void *))
{
	int i = 0;

	if(!threadfunc) {
		logerr("threadfunc is NULL");
		return -1;
	} else {
		threads->threadfunc = threadfunc;
	}
	if(!threads->size) {
		threads->size = WORKER_POOL_SIZE;
	}
	if(!threads->grow) {
		threads->grow = WORKER_POOL_GROW;
	}
	if(!threads->mode) {
		threads->mode = WORKER_QUEUE_MODE;
	}
	if(!threads->max) {
		threads->max = WORKER_POOL_MAX;
	}
	if(!threads->wlimit) {
		threads->wlimit = WORKER_WAIT_LIMIT;
	}
	if(!threads->wsleep) {
		threads->wsleep = WORKER_WAIT_SLEEP;
	}
	threads->pool = malloc(sizeof(pthread_t) * threads->size);
	if(!threads->pool) {
		return -1;
	}	
	worker_show_stats("init", threads);

	threads->used = 0;
	threads->data = data;

	dllist_init(&threads->ready, NULL, worker_ready_compare);
	debug("initilized ready list (currently %d peers)", dllist_count(&threads->ready));
		
	if(pthread_mutex_init(&threads->predlock, NULL) != 0) {
		logerr("failed init mutex");
		return -1;
	}
	debug("initilized predict mutex");
	if(pthread_mutex_init(&threads->poollock, NULL) != 0) {
		logerr("failed init mutex");
		return -1;
	}
	debug("initilized thread pool mutex");
	if(pthread_mutex_init(&threads->peerlock, NULL) != 0) {
		logerr("failed init mutex");
		return -1;
	}		
	debug("intilized peer manager mutex");
	if(pthread_cond_init(&threads->peercond, NULL) != 0) {
		logerr("failed init condition");
		return -1;
	}
	debug("initilized peer wait condition");
	
	debug("creating threads");
	for(i = 0; i < threads->size; ++i) {
		pthread_mutex_lock(&threads->peerlock);
		if(pthread_create(&threads->pool[i], NULL, threads->threadfunc, threads) != 0) {
			logerr("failed create thread");
			pthread_mutex_unlock(&threads->peerlock);
			return -1;
		}
		pthread_mutex_unlock(&threads->peerlock);
		debug("created thread 0x%lu", threads->pool[i]);
	}
	debug("finished initilize worker thread manager");
	
	return 0;
}

/*
 * Insert peer socket in ready list and wake up worker thread, possibly enlarge 
 * the list of worker threads. Returns -1 on failure and sets the errno variable.
 * On success 0 is returned. 
 * 
 * If the thread pool is at maximum size and no worker thread is available to
 * emidiate handle the peer then examine the mode variable to decide if we should 
 * enqueue the peer and let it be on hold until a worker thread become available or
 * if we should refuse this connection (with server busy error).
 * 
 * NOTE: this function is called on behalf of the main thread.
 */
int worker_enqueue(struct workers *threads, int sock, struct options *popt, const struct cgps_project *proj)
{
	struct client *peer;

	if(opts->debug > 1) {
		worker_show_stats("enqueue", threads);
	}
	
	pthread_mutex_lock(&threads->poollock);
	if(!threads->size) {
		pthread_mutex_unlock(&threads->poollock);
		errno = EINVAL;
		logerr("thread pool size is zero");
		return -1;
	} else if(threads->used < threads->size) {
		debug("thread pool has %d unused workers", threads->size - threads->used);
		pthread_mutex_unlock(&threads->poollock);
	} else if(threads->size <= threads->max - threads->grow) {
		pthread_t *newpool;
		int i, newsize;

		newsize = threads->size + threads->grow;
		debug("thread pool needs to grow: size %d -> %d", threads->size, newsize);
		
		newpool = realloc(threads->pool, sizeof(pthread_t) * newsize);
		if(!newpool) {
			pthread_mutex_unlock(&threads->poollock);
			logerr("failed grow thread pool");
			return -1;
		}
		threads->pool = newpool;
		
		debug("creating threads");
		for(i = threads->size; i < newsize; ++i) {
			pthread_mutex_lock(&threads->peerlock);
			if(pthread_create(&threads->pool[i], NULL, threads->threadfunc, threads) != 0) {
				pthread_mutex_unlock(&threads->peerlock);
				pthread_mutex_unlock(&threads->poollock);
				logerr("failed create thread");
				return -1;
			}
			pthread_mutex_unlock(&threads->peerlock);
			debug("created thread 0x%lu", threads->pool[i]);
			threads->size++;
		}
		pthread_mutex_unlock(&threads->poollock);
	} else {
		pthread_mutex_unlock(&threads->poollock);
		if(threads->mode == WORKER_QUEUE_NONE) {
			errno = EBUSY;
			logerr("all worker threads are busy");
			return -1;
		}
		debug("queue pending peer until worker thread becomes available");
	}
	
	peer = malloc(sizeof(struct client));
	if(!peer) {
		logerr("failed alloc memory");
		return -1;
	}
	memset(peer, 0, sizeof(struct client));
	
	peer->sock = sock;
	peer->opts = popt;
	peer->proj = proj;
	
	pthread_mutex_lock(&threads->peerlock);
	dllist_insert(&threads->ready, peer, DLL_INSERT_TAIL);
	pthread_cond_signal(&threads->peercond);
	pthread_mutex_unlock(&threads->peerlock);
	
	return 0;
}

/*
 * Dequeue a ready peer socket from the ready list. Returns NULL if no ready
 * peers exists. 
 * 
 * NOTE: this function gets called from a worker thread.
 */
struct client * worker_dequeue(struct workers *threads)
{
	struct client *peer = NULL;

	if(opts->debug > 1) {
		worker_show_stats("dequeue", threads);
	}
	
	pthread_mutex_lock(&threads->peerlock);
	peer = dllist_move_first(&threads->ready);
	if(peer) {
		dllist_remove(&threads->ready);
	} 
	pthread_mutex_unlock(&threads->peerlock);

	if(peer) {
		debug("peer dequeued from ready list (sock %d)", peer->sock);
		pthread_mutex_lock(&threads->poollock);
		threads->used++;
		debug("incremented worker usage (%d used)", threads->used); 
		pthread_mutex_unlock(&threads->poollock);
	}
	
	return peer;
}

/*
 * Called from threadfunc() to flag worker as unused.
 * 
 * NOTE: this function gets called from a worker thread.
 */
void worker_release(struct workers *threads)
{
	if(opts->debug > 1) {
		worker_show_stats("release", threads);
	}
	
	pthread_mutex_lock(&threads->poollock);
	threads->used--;
	debug("decremented worker usage (%d used)", threads->used); 
	pthread_mutex_unlock(&threads->poollock);
}

/*
 * Join all threads and release resources.
 */
void worker_cleanup(struct workers *threads)
{
	struct client *peer = NULL;
	int i;

	debug("cleanup worker threads...");

	if(opts->debug > 1) {
		worker_show_stats("cleanup", threads);
	}
	
	pthread_mutex_lock(&threads->peerlock);
	debug("broadcast peer condition to blocked threads");
	pthread_cond_broadcast(&threads->peercond);
	pthread_mutex_unlock(&threads->peerlock);
	
	for(i = 0; i < threads->size; ++i) {
		if(pthread_join(threads->pool[i], NULL) != 0) {
			logerr("failed join thread 0x%lu", threads->pool[i]);
		}
		debug("joined thread 0x%lu (%d/%d)", threads->pool[i], i + 1, threads->size);
	}
	
	pthread_mutex_lock(&threads->poollock);
	free(threads->pool);
	threads->pool = NULL;
	threads->size = threads->used = 0;
	pthread_mutex_unlock(&threads->poollock);
	debug("released thread pool resources");

	for(peer = dllist_move_first(&threads->ready); peer; peer = dllist_move_next(&threads->ready)) {
		worker_ready_destroy(peer);
	}
	dllist_free(&threads->ready);
	
	pthread_mutex_destroy(&threads->predlock);
	debug("destroyed predict mutex");
	pthread_mutex_destroy(&threads->poollock);
	debug("destroyed thread pool mutex");
	pthread_mutex_destroy(&threads->peerlock);
	debug("destroyed peer manager mutex");
	pthread_cond_destroy(&threads->peercond);
	debug("destroyed peer wait condition");	
}

/*
 * Returns number of peers waiting in queue.
 */
int worker_waiting(struct workers *threads)
{
	int count = 0;
	
	pthread_mutex_lock(&threads->peerlock);
	count = dllist_count(&threads->ready);
	pthread_mutex_unlock(&threads->peerlock);

	return count;
}
