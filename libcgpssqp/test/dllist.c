/* Monitors and restarts a crashed child process.
 * Copyright (C) 2007 Anders Lövgren
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
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define NDEBUG

#define DLL_NUM_NODES 50000000

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "alived.h"
#include "dllist.h"

struct data
{
	char *name;
	unsigned int age;
};

struct options *opts;

/*
 * Destroy object callback.
 */
/*
void destroy(void *data)
{
	struct data *d = (struct data *)data;
	
	debug("destroy called for name=%s (age=%d)\n", d->name, d->age);

	if(d) {
		free(d->name);
		free(d);
		d = NULL;
	}
}
*/

/*
 * Destroy int callback.
 */
void destroy(void *data)
{
	int *i = (int *)data;
	(*i)++;
}

/*
 * Compare objects callback (compares by age).
 */
int compare(void *data1, void *data2)
{
	struct data *d1, *d2;
	
	d1 = (struct data *)data1;
	d2 = (struct data *)data2;

	debug("compare_age() called for name=%s (age=%d) and name=%s (age=%d)",
	      d1->name, d1->age, d2->name, d2->age);
	
	return d1->age - d2->age;
}

/*
 * Create data for one list node.
 */
struct data * create_data(const char *name, unsigned int age)
{
	struct data *d;
	
	d = malloc(sizeof(struct data));
	if(!d) {
		die("failed alloc memory");
	}

	d->name = malloc(strlen(name) + 1);
	if(!d->name) {
		free(d);
		die("failed alloc memory");
	}	
	strcpy(d->name, name);
	d->age = age;
	
	return d;
}

void dump_data(struct data *data, const char *msg)
{
	loginfo("    name: %s (age: %d) %s", data->name, data->age, msg);
}

void dump_list(struct dllist *list)
{
	struct data *data;

	loginfo("    dumping list (count=%d):", dllist_count(list));
	loginfo("    ------------------------------------");
	data = dllist_move_first(list);
	while(data) {
		dump_data(data, "[data]");
		data = dllist_move_next(list);
	}
	loginfo("");
}

float clock_diff(clock_t start, clock_t end)
{
	return ((float)end - (float)start) / CLOCKS_PER_SEC;
}

int main(int argc, char **argv)
{
	struct dllist list;
	struct data *data;
	struct data *p27a, *p27b, *p36a, *p36b, *p36c, *p52a, *p52b, *p53a;
	int i, val;
	clock_t start, end;
	unsigned long bytes;
	
	opts = malloc(sizeof(struct options));
	if(!opts) {
		die("failed alloc memory");
	}
	memset(opts, 0, sizeof(struct options));

# ifndef NDEBUG
	opts->debug = 1;
# endif
	opts->prog  = argv[0];

	p27a = create_data("gustav", 27);
	p27b = create_data("ivar", 27);
	p36a = create_data("adam", 36);
	p36b = create_data("bertil", 36);
	p36c = create_data("ceasar", 36);
	p52a = create_data("henrik", 52);
	p52b = create_data("johan", 52);
	p53a = create_data("karl", 53);
	
	loginfo("*** Dumping empty list:");
	loginfo("----------------------------------------");
	dllist_init(&list, NULL, compare);
	dump_list(&list);
	dllist_free(&list);

	loginfo("*** Insert at front:");
	loginfo("----------------------------------------");
	dllist_init(&list, NULL, compare);
	dllist_insert(&list, p36a, DLL_INSERT_HEAD);
	dllist_insert(&list, p52a, DLL_INSERT_HEAD);
	dllist_insert(&list, p27a, DLL_INSERT_HEAD);
	dump_list(&list);
	dllist_free(&list);

	loginfo("*** Insert at back:");
	loginfo("----------------------------------------");
	dllist_init(&list, NULL, compare);
	dllist_insert(&list, p36a, DLL_INSERT_TAIL);
	dllist_insert(&list, p52a, DLL_INSERT_TAIL);
	dllist_insert(&list, p27a, DLL_INSERT_TAIL);
	dump_list(&list);
	dllist_free(&list);

	loginfo("*** Insert at current (same as insert at tail):");
	loginfo("----------------------------------------");
	dllist_init(&list, NULL, compare);
	dllist_insert(&list, p36a, DLL_INSERT_CURR);
	dllist_insert(&list, p52a, DLL_INSERT_CURR);
	dllist_insert(&list, p27a, DLL_INSERT_CURR);
	dump_list(&list);

	loginfo("*** Move first and remove all nodes:");
	loginfo("----------------------------------------");
	if(dllist_move_first(&list)) {
		while(dllist_remove(&list) != -1);
	}
	dump_list(&list);	
	dllist_free(&list);

	loginfo("*** Find first (where=0):");
	loginfo("----------------------------------------");
	dllist_init(&list, NULL, compare);
	dllist_insert(&list, p36a, DLL_INSERT_TAIL);
	dllist_insert(&list, p52a, DLL_INSERT_TAIL);
	dllist_insert(&list, p27a, DLL_INSERT_TAIL);
	dump_list(&list);

	data = dllist_find(&list, p27b, NULL, 0);
	if(data) {
		dump_data(data, "[found]");
	} else {
		loginfo("    dllist_find() returned null [failed]");
	}
	data = dllist_find(&list, p53a, NULL, 0);
	if(data) {
		dump_data(data, "[found]");
	} else {
		loginfo("    dllist_find() returned null [failed]");
	}
	data = dllist_find(&list, p36b, NULL, 0);
	if(data) {
		dump_data(data, "[found]");
	} else {
		loginfo("    dllist_find() returned null [failed]");
	}
	data = dllist_find(&list, p52b, NULL, 0);
	if(data) {
		dump_data(data, "[found]");
	} else {
		loginfo("    dllist_find() returned null [failed]");
	}
	loginfo("");
	
	loginfo("*** Remove last dllist_find() result:");
	loginfo("----------------------------------------");
	dllist_remove(&list);
	dump_list(&list);	
	dllist_free(&list);

	loginfo("*** Iterate:");
	loginfo("----------------------------------------");
	dllist_init(&list, NULL, compare);
	dllist_insert(&list, p36a, DLL_INSERT_TAIL);
	dllist_insert(&list, p52a, DLL_INSERT_TAIL);
	dllist_insert(&list, p27a, DLL_INSERT_TAIL);
	dump_list(&list);
	loginfo("    inorder:");
	loginfo("    ------------------------------------");
	data = dllist_move_first(&list);
	while(data) {
		dump_data(data, "");
		data = dllist_move_next(&list);
	}
	loginfo("");
	loginfo("    reversed:");
	loginfo("    ------------------------------------");
	data = dllist_move_last(&list);
	while(data) {
		dump_data(data, "");
		data = dllist_move_prev(&list);
	}
	loginfo("");
	dllist_free(&list);

	loginfo("*** Find all matches:");
	loginfo("----------------------------------------");
	dllist_init(&list, NULL, compare);
	dllist_insert(&list, p36a, DLL_INSERT_TAIL);
	dllist_insert(&list, p36b, DLL_INSERT_TAIL);
	dllist_insert(&list, p27a, DLL_INSERT_TAIL);
	dllist_insert(&list, p36c, DLL_INSERT_TAIL);
	dllist_insert(&list, p52a, DLL_INSERT_TAIL);
	dump_list(&list);
	loginfo("    using find_first/find_next:");
	loginfo("    ------------------------------------");
	data = dllist_find_first(&list, p36c);
	while(data) {
		dump_data(data, "[found]");
		data = dllist_find_next(&list);
	}
	loginfo("");
	loginfo("    using find_first/find_more:");
	loginfo("    ------------------------------------");
	data = dllist_find_first(&list, p36c);
	while(data) {
		dump_data(data, "[found]");
		data = dllist_find_more(&list, p52a);
	}
	loginfo("");
	loginfo("    using find (forward):");
	loginfo("    ------------------------------------");
	data = dllist_find(&list, p36c, NULL, DLL_SEEK_HEAD | DLL_SEEK_NEXT);
	while(data) {
		dump_data(data, "[found]");
		data = dllist_find(&list, p36c, NULL, DLL_SEEK_CURR | DLL_SEEK_NEXT);
	}
	loginfo("");
	loginfo("    using find (reverse):");
	loginfo("    ------------------------------------");
	data = dllist_find(&list, p36c, NULL, DLL_SEEK_TAIL | DLL_SEEK_PREV);
	while(data) {
		dump_data(data, "[found]");
		data = dllist_find(&list, p36c, NULL, DLL_SEEK_CURR | DLL_SEEK_PREV);
	}
	loginfo("");
	dllist_free(&list);

	bytes = DLL_NUM_NODES * sizeof(struct dllnode) + sizeof(struct dllist);
	
	loginfo("*** Performace test (%d nodes allocating %lu MB):", DLL_NUM_NODES, bytes / (1024 * 1024));
	loginfo("----------------------------------------");

	/*
	 * Run one dummy test first so that the CPU scales up to max
	 * freq on computers with CPU freq scaling. Otherwise we will 
	 * get false poor performance on first test.
	 */
	dllist_init(&list, NULL, NULL);
	val = 0;
	fprintf(stderr, "    prepare for performance tests: ");
	for(i = 0; i < DLL_NUM_NODES; ++i) {
		dllist_insert(&list, &val, DLL_INSERT_HEAD);
	}
	if(dllist_count(&list) != DLL_NUM_NODES) {
		die("number of inserted nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done\n");
	dllist_free(&list);
	
	dllist_init(&list, NULL, NULL);
	val = 0;
	fprintf(stderr, "    testing list insert (head): ");
	start = clock();
	for(i = 0; i < DLL_NUM_NODES; ++i) {
		dllist_insert(&list, &val, DLL_INSERT_HEAD);
	}
	end = clock();
	if(dllist_count(&list) != DLL_NUM_NODES) {
		die("number of inserted nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));	
	dllist_free(&list);
	
	dllist_init(&list, NULL, NULL);
	val = 0;
	fprintf(stderr, "    testing list insert (tail): ");
	start = clock();
	for(i = 0; i < DLL_NUM_NODES; ++i) {
		dllist_insert(&list, &val, DLL_INSERT_TAIL);
	}
	end = clock();
	if(dllist_count(&list) != DLL_NUM_NODES) {
		die("number of inserted nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));	
	dllist_free(&list);
	
	dllist_init(&list, NULL, NULL);
	val = 0;
	fprintf(stderr, "    testing list insert (curr): ");
	start = clock();
	for(i = 0; i < DLL_NUM_NODES; ++i) {
		dllist_insert(&list, &val, DLL_INSERT_CURR);
	}
	end = clock();
	if(dllist_count(&list) != DLL_NUM_NODES) {
		die("number of inserted nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));	
	dllist_free(&list);
	
	dllist_init(&list, NULL, NULL);
	val = 0;
	fprintf(stderr, "    testing list remove (no callback):\n");
	fprintf(stderr, "      -> inserting:  ");
	start = clock();
	for(i = 0; i < DLL_NUM_NODES; ++i) {
		dllist_insert(&list, &val, DLL_INSERT_CURR);
	}
	end = clock();
	if(dllist_count(&list) != DLL_NUM_NODES) {
		die("number of inserted nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));
	fprintf(stderr, "      -> removing:   ");
	i = 0;
	dllist_move_first(&list);
	start = clock();
	while(dllist_remove(&list) != -1) {
		++i;
	}
	end = clock();
	if(i != DLL_NUM_NODES) {
		die("number of removed nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));	
	dllist_free(&list);

	dllist_init(&list, destroy, NULL);
	val = 0;
	fprintf(stderr, "    testing list remove (using destroy):\n");
	fprintf(stderr, "      -> inserting:  ");
	start = clock();
	for(i = 0; i < DLL_NUM_NODES; ++i) {
		dllist_insert(&list, &val, DLL_INSERT_CURR);
	}
	end = clock();
	if(dllist_count(&list) != DLL_NUM_NODES) {
		die("number of inserted nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));
	fprintf(stderr, "      -> removing:   ");
	i = 0;
	dllist_move_first(&list);
	start = clock();
	while(dllist_remove(&list) != -1) {
		++i;
	}
	end = clock();
	if(i != DLL_NUM_NODES) {
		die("number of removed nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));	
	dllist_free(&list);
	
	dllist_init(&list, NULL, NULL);
	val = 0;
	fprintf(stderr, "    testing list destroy (no callback):\n");
	fprintf(stderr, "      -> inserting:  ");
	start = clock();
	for(i = 0; i < DLL_NUM_NODES; ++i) {
		dllist_insert(&list, &val, DLL_INSERT_CURR);
	}
	end = clock();
	if(dllist_count(&list) != DLL_NUM_NODES) {
		die("number of inserted nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));
	fprintf(stderr, "      -> destroying: ");
	start = clock();
	dllist_free(&list);
	end = clock();
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));	
	
	dllist_init(&list, destroy, NULL);
	val = 0;
	fprintf(stderr, "    testing list destroy (using destroy):\n");
	fprintf(stderr, "      -> inserting:  ");
	start = clock();
	for(i = 0; i < DLL_NUM_NODES; ++i) {
		dllist_insert(&list, &val, DLL_INSERT_CURR);
	}
	end = clock();
	if(dllist_count(&list) != DLL_NUM_NODES) {
		die("number of inserted nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));
	fprintf(stderr, "      -> destroying: ");
	start = clock();
	dllist_free(&list);
	end = clock();
	if(val != DLL_NUM_NODES) {
		die("number of destroyed nodes != %d", DLL_NUM_NODES);
	}
	fprintf(stderr, "done [%.03f sec]\n", clock_diff(start, end));	
	
	free(p27a); free(p27b); free(p36a); free(p36b); 
	free(p36c); free(p52a); free(p52b); free(p53a);

	free(opts);
	return 0;
}
