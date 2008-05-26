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

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>
#endif

#include "cgpsddos.h"
#include "cgpssqp.h"

/*
 * Compare the hostname:service of two slave nodes.
 */
static int hostname_compare(void *data1, void *data2)
{
	struct cgpspeer *peer1 = (struct cgpspeer *)data1;
	struct cgpspeer *peer2 = (struct cgpspeer *)data2;
	int res;
	
	res = strcmp(peer1->host, peer2->host);
	if(!res) {
		return 0;
	}
	return strcmp(peer1->serv, peer2->serv);
}

/*
 * Cleanup resolve data.
 */
static void resolve_cleanup(void *args)
{
	struct resolve_slave *res = (struct resolve_slave *)args;
	free(res->data);
	free(res);
}

/*
 * Resolve hostname. This function returns a void pointer to an addrinfo 
 * structure filled with info about resolved hostname. The args argument 
 * should point to struct resolve_data data that is filled with reverse 
 * lookup hostname and service. Call freeaddrinfo() on return value
 * to free its allocated memory.
 */
void * resolve_host(void *args)
{
	struct resolve_data *res = (struct resolve_data *)args;
	struct addrinfo hints, *addr, *next = NULL;

	hints.ai_family = res->ddos->family;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_ADDRCONFIG;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	
	if(getaddrinfo(res->peer->host, res->peer->port, &hints, &addr) == 0) {
		for(next = addr; next != NULL; next = next->ai_next) {
			if(!res->peer->host) {
				res->peer->host = malloc(NI_MAXHOST);
				res->peer->serv = malloc(NI_MAXSERV);
				if(!res->peer->host || !res->peer->serv) {
					logerr("failed alloc memory");
					return NULL;
				}
			}
			if(getnameinfo(next->ai_addr,
				       next->ai_addrlen, 
				       res->peer->host, NI_MAXHOST,
				       res->peer->serv, NI_MAXSERV, NI_NUMERICSERV) == 0) {
				debug("reverse lookup of %s:%s is %s:%s", res->peer->addr, res->peer->port, res->peer->host, res->peer->serv);
				break;
			}
		}
		res->peer->ainf = addr;
		return addr;
	} 
	if(!addr) {
		logerr("failed resolve peer address of %s", res->peer->addr);
	}
	if(!next) {
		logerr("failed lookup service %s", res->peer->port);
	}
	return NULL;
}

/*
 * Resolve all slaves in list read from file stream fs. Run each resolve in
 * its own thread to speedup the resolve process on lame DNS servers.
 */
void resolve_slaves(struct cgpsddos *ddos, struct dllist *slaves, FILE *fs)
{
	struct dllist threads;
	char *buff = NULL;
	size_t size = 0;
	ssize_t bytes;

	debug("initilize list of threads");
	dllist_init(&threads, resolve_cleanup, NULL);
	
	while((bytes = getline(&buff, &size, fs)) != -1) {
		struct resolve_slave *res;
		char *addr, *port;

		res = malloc(sizeof(struct resolve_slave));
		if(!res) {
			die("failed alloc memory");
		}
		memset(res, 0, sizeof(struct resolve_slave));
		
		res->data = malloc(sizeof(struct resolve_data));
		if(!res->data) {
			die("failed alloc memory");
		}
		memset(res->data, 0, sizeof(struct resolve_data));

		res->data->peer = malloc(sizeof(struct cgpspeer));
		if(!res->data->peer) {
			die("failed alloc memory");
		}
		memset(res->data->peer, 0, sizeof(struct cgpspeer));
		
		split_hostaddr(buff, &addr, &port);
		if(addr) {
			res->data->peer->addr = strdup(addr);
		}
		if(port) {
			res->data->peer->port = strdup(port);
		}		
		if(!res->data->peer->port) {
			res->data->peer->port = malloc(6);
			if(!res->data->peer->port) {
				die("failed alloc memory");
			}
			snprintf(res->data->peer->port, 5, "%d", CGPSDDOS_SLAVE_PORT);
		}
		if(!res->data->peer->addr || !res->data->peer->port) {
			die("failed alloc memory");
		}
		res->data->ddos = ddos;
		
		if(pthread_create(&res->thread, NULL, resolve_host, res->data) != 0) {
			logerr("failed create thread");
			continue;
		}
		if(dllist_insert(&threads, res, DLL_INSERT_TAIL) < 0) {
			logerr("failed add to thread list");
			pthread_join(res->thread, NULL);
			continue;
		}
		debug("thread 0x%x: starting resolve %s:%s", 
		      res->thread, 
		      res->data->peer->addr, 
		      res->data->peer->port);
	}
	free(buff);

	debug("joining resolver threads");
	if(dllist_count(&threads)) {
		struct resolve_slave *res;
		for(res = dllist_move_first(&threads); res; res = dllist_move_next(&threads)) {
			struct addrinfo *addr;
			if(pthread_join(res->thread, (void *)&addr) != 0)  {
				logerr("failed join thread");
			} else {
				debug("thread 0x%x: joined resolve thread", res->thread);				
				if(addr) {
					if(dllist_find(slaves, res->data->peer, hostname_compare, DLL_SEEK_HEAD)) {
						logwarn("skipped duplicate hostname %s:%s (%s:%s) (already in the hosts list)",
							res->data->peer->host, res->data->peer->serv,
							res->data->peer->addr, res->data->peer->port);
					} else {
						debug("thread 0x%x: inserting resolved address", res->thread);
						dllist_insert(slaves, res->data->peer, DLL_INSERT_TAIL);
					}
				}
			}
		}
	}
	debug("all resolver threads joined");
	
	debug("cleaning up resolver threads list");
	dllist_free(&threads);	
}

/*
 * Split the address string into host/port components. The addr argument
 * gets modified by calling this function. The addr uses ':' as separator.
 */
char * split_hostaddr(char *addr, char **host, char **port)
{
	*host = *port = NULL;
	if(addr) {
		if(strchr(addr, '\n')) {
			char *last = strrchr(addr, '\n');
			while(*last == '\n') {
				*last-- = '\0';
			}
		}
		*host = addr;
		*port = strchr(*host, ':');
		if(*port) {
			*(*port)++ = '\0';
		}
	}
	return addr;
}
