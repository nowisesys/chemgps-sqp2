/* SIMCA-QP predictions for the ChemGPS project.
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#include <sys/un.h>

#include "cgpssqp.h"
#include "cgpsclt.h"

/*
 * Initilize the server connection (both IPv4 and IPv6 supported).
 */
int init_socket(struct options *popt)
{
	if(popt->ipaddr) {
		struct addrinfo hints, *addr, *next = NULL;
		int retries = 0, res;
		char port[7];
		char host[NI_MAXHOST], serv[NI_MAXSERV];
		int errval = 0;
		
		snprintf(port, sizeof(port) - 1, "%d", popt->port);

		hints.ai_family = popt->family;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_ADDRCONFIG;
		hints.ai_protocol = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;
		
		while(retries < CGPS_RESOLVE_RETRIES) {
			if((res = getaddrinfo(popt->ipaddr, port, &hints, &addr)) != 0) {
				if(res == EAI_AGAIN) {
					if(!popt->quiet) {
						logwarn("temporary failure resolving hostname %s (%s)", 
							popt->ipaddr, gai_strerror(res)); 
					}
				} else {
					if(!popt->quiet) {
						logerr("failed resolve %s:%d (%s)", 
						       popt->ipaddr, popt->port, gai_strerror(res));
					}
					return CGPSCLT_CONN_FAILED;
				}
				if(popt->noretry) {
					freeaddrinfo(addr);
					return CGPSCLT_CONN_FAILED;
				}
				++retries;
				sleep(CGPS_RESOLVE_TIMEOUT);
			} else {
				break;
			}
		}
		
		for(next = addr; next != NULL; next = next->ai_next) {
			popt->ipsock = socket(next->ai_family, next->ai_socktype, next->ai_protocol);
			if(popt->ipsock < 0) {
				continue;
			} 
			if(connect(popt->ipsock, next->ai_addr, next->ai_addrlen) < 0) {
				errval = errno;
				close(popt->ipsock);
				continue;
			}
			break;
		}
		if(!next) {
			freeaddrinfo(addr);
			if(popt->ipsock < 0) {
				if(!popt->quiet) {
					logerr("failed create TCP socket");
				}
				return CGPSCLT_CONN_FAILED;
			} else {
				if(errval == ETIMEDOUT) {
					debug("timeout connecting to server %s (retrying)", popt->ipaddr);
					return CGPSCLT_CONN_RETRY;
				} else {
					if(!popt->quiet) {
						logerr("failed connect to %s:%d", popt->ipaddr, popt->port);
					}
					return CGPSCLT_CONN_FAILED;
				}
			}
		}

		if(popt->debug) {
			if(getnameinfo(next->ai_addr,
				       next->ai_addrlen, 
				       host, NI_MAXHOST,
				       serv, NI_MAXSERV, NI_NUMERICSERV) == 0) {
				debug("connected to %s:%s (%s:%d) (%s)", 
				      host, serv, popt->ipaddr, popt->port, 
				      next->ai_family == AF_INET ? "ipv4" : "ipv6");
			} else {
				debug("connected to %s:%d (%s)", 
				      popt->ipaddr, popt->port, 
				      next->ai_family == AF_INET ? "ipv4" : "ipv6");
			}
		}
		freeaddrinfo(addr);
		return CGPSCLT_CONN_SUCCESS;
	}
	if(popt->unaddr) {
		struct sockaddr_un sockaddr;
		
		popt->unsock = socket(PF_UNIX, SOCK_STREAM, 0);
		if(popt->unsock < 0) {
			if(!popt->quiet) {
				logerr("failed create UNIX socket");
			}
			return CGPSCLT_CONN_FAILED;
		}
		debug("created UNIX socket");
		
		sockaddr.sun_family = AF_UNIX;
		strncpy(sockaddr.sun_path, popt->unaddr, sizeof(sockaddr.sun_path));
		if(connect(popt->unsock, 
			   (const struct sockaddr *)&sockaddr,
			   sizeof(struct sockaddr_un)) < 0) {
			if(errno == ETIMEDOUT) {
				debug("timeout connecting to socket %s (retrying)", popt->unaddr);
				close(popt->unsock);
				return CGPSCLT_CONN_RETRY;
			} else {
				if(!popt->quiet) {
					logerr("failed connect to %s", popt->unaddr);
				}
				close(popt->unsock);
				return CGPSCLT_CONN_FAILED;
			}
		}
		debug("connected to %s", popt->unaddr);
		return CGPSCLT_CONN_SUCCESS;
	}
	return CGPSCLT_CONN_FAILED;
}
