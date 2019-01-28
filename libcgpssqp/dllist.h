/* Simca-QP predictions for the ChemGPS project.
 *
 * Copyright (C) 2007-2018 Anders Lövgren and the Computing Department,
 * Uppsala Biomedical Centre, Uppsala University.
 * 
 * Copyright (C) 2018-2019 Anders Lövgren, Nowise Systems
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
 *  Contact: Anders Lövgren <andlov@nowise.se>
 * ----------------------------------------------------------------------
 */

/*
 * Double linked list.
 */

#ifndef __DLLIST_H__
#define __DLLIST_H__

/*
 * Used as where argument for dllist_add()
 */
#define DLL_INSERT_HEAD 0
#define DLL_INSERT_TAIL 1
#define DLL_INSERT_CURR 2

/*
 * Used as seek argument for dllist_find()
 */
#define DLL_SEEK_HEAD 1        /* start at head */
#define DLL_SEEK_TAIL 2        /* start at tail */
#define DLL_SEEK_CURR 4        /* start at current position */
#define DLL_SEEK_NEXT 8        /* seek forward */
#define DLL_SEEK_PREV 16       /* seek backward */
/*
 * Alias for forward search for dllist_find()
 */
#define DLL_SEEK_START  (DLL_SEEK_HEAD | DLL_SEEK_NEXT)  /* first search */
#define DLL_SEEK_CONT   (DLL_SEEK_CURR | DLL_SEEK_NEXT)  /* subseq search */

/*
 * A node in the list.
 */
struct dllnode
{
	struct dllnode *prev;
	struct dllnode *next;
	void *data;
};

/*
 * The double linked list.
 */
struct dllist
{
	void (*destroy)(void *);         /* destroy callback */
	int (*compare)(void *, void *);  /* compare callback */
	struct dllnode *head;
	struct dllnode *tail;
	struct dllnode *curr;            /* iterator */
	void *data;                      /* search predicate */
	unsigned int count;
};

/*
 * Initilize the list. If destroy is not NULL, then it is called with
 * data pointer for each node that is removed. The compare callback is
 * called by dllist_find().
 */
void dllist_init(struct dllist *, void (*destroy)(void *), int (*compare)(void *, void *));

/*
 * Returns number of nodes in the list.
 */
static __inline__ unsigned int dllist_count(struct dllist *);

/*
 * Find data in the list using compare as callback function. The compare
 * function should return 0 on match. See note above for description of
 * the seek argument.
 */
void * dllist_find(struct dllist *, void *data, int (*compare)(void *, void *), int seek);

/*
 * Move iterator to first node and return its data.
 */
void * dllist_move_first(struct dllist *);

/*
 * Move iterator to last node and return its data.
 */
void * dllist_move_last(struct dllist *);

/*
 * Move iterator to next node and return its data.
 */
void * dllist_move_next(struct dllist *);

/*
 * Move iterator to previous node and return its data.
 */
void * dllist_move_prev(struct dllist *);

/*
 * Insert data in the list. The where argument is one of the defines above.
 * This function returns -1 on failure and 0 if successful.
 */
int dllist_insert(struct dllist *, void *data, int where);

/*
 * Removes the current list node. This function returns -1 on failure and
 * 0 if successful. The only reason for dllist_remove() to fail is that
 * current list node is NULL. Use  dllist_find() or dllist_move_xxx() to
 * set current list node.
 */
int dllist_remove(struct dllist *);

/*
 * Release memory allocated for list nodes. If destroy in set, then its
 * gets called for each nodes data.
 */
void dllist_free(struct dllist *);

/*
 * Shorthand versions of the more generic dllist_find(). The search is done
 * forward from the lists head to its tail. Allways call dllist_find() or
 * dllist_find_first() before using dllist_find_next(). The dllist_find_more() 
 * macro allows its caller to refine the data searched for without starting 
 * a new search.
 */
#define dllist_find_first(list, data) dllist_find((list), (data), NULL, DLL_SEEK_START)
#define dllist_find_next(list) dllist_find((list), NULL, NULL, DLL_SEEK_CONT)
#define dllist_find_more(list, data) dllist_find((list), (data), NULL, DLL_SEEK_CONT)

unsigned int dllist_count(struct dllist *q)
{
	return q->count;
}

#endif  /* __DLLIST_H__ */
