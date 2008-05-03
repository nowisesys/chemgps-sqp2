/* Simca-QP predictions for the ChemGPS project.
 *
 * Copyright (C) 2007-2008 Anders L�vgren and the Computing Department,
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
 *  Contact: Anders L�vgren <anders.lovgren@bmc.uu.se>
 * ----------------------------------------------------------------------
 */

/*
 * The interface for the thread pool (workers).
 */

#ifndef __WORKER_H__
#define __WORKER_H__

#define WORKER_POOL_SIZE  5            /* workers size hint */
#define WORKER_POOL_GROW  5            /* workers grow hint */
#define WORKER_POOL_MAX  20            /* maximum workers hint */

/*
 * Wait for worker thread queue policy (for workers->mode). This defines
 * the enqueue policy when all working threads are busy.
 */
#define WORKER_QUEUE_WAIT 1            /* enqueue peer waiting for a worker thread */
#define WORKER_QUEUE_NONE 2            /* don't enqueue peer waiting for worker */
#define WORKER_QUEUE_MODE WORKER_QUEUE_WAIT

struct workers
{
	void * (*threadfunc)(void *);  /* thread start function */
	pthread_t *pool;               /* thread pool */
	pthread_mutex_t poollock;      /* lock access to pool */
	pthread_mutex_t peerlock;      /* lock access to peer list */
	pthread_cond_t peercond;       /* peer wait condition */
	int size;                      /* number of workers */
	int used;                      /* used workers */
	int grow;                      /* grow size of workers */
	int max;                       /* maximum number of workers */
	int mode;                      /* see WORKER_QUEUE_XXX */
	void *data;                    /* common work thread data */
	struct dllist ready;           /* list of ready peers */
};

/*
 * Initilize the pool of threads (workers). This function should only be
 * called once and worker_cleanup() should be called to cleanup resources
 * allocated in threads.
 */
int worker_init(struct workers *threads, void *data, void * (*threadfunc)(void *));

/*
 * Insert peer socket in ready list and wake up worker thread, possibly enlarge 
 * the list of worker threads. Returns -1 on failure and sets the errno variable.
 * On success 0 is returned.
 */
int worker_enqueue(struct workers *threads, int sock, const struct options *opts, const struct cgps_project *proj);

/*
 * Dequeue a ready peer socket from the ready list. Returns a pointer to next
 * ready client (might be NULL if no ready peers exists).
 */
struct client * worker_dequeue(struct workers *threads);

/*
 * Called from threadfunc() to flag worker as unused.
 */
void worker_release(struct workers *threads);

/*
 * Join all threads and release resources.
 */
void worker_cleanup(struct workers *threads);

#endif /* __WORKER_H__ */