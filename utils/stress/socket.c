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
#include "stress.h"

/*
 * The code should be fixed to work with IPv4/IPv6 addresses if
 * the gethostbyname() function is missing.
 */
#if ! defined(HAVE_GETHOSTBYNAME) || HAVE_GETHOSTBYNAME == 0
# error "The gethostbyname() function is missing, please rewrite init_socket()" 
#endif

/*
 * Initilize the server connection.
 */
void init_socket(struct options *opts)
{
	if(opts->ipaddr) {		
		struct hostent *ent = NULL;
		int retries = 0;
		
		while(retries < CGPSCLT_RESOLVE_RETRIES) {
			ent = gethostbyname(opts->ipaddr);
			if(!ent) { 
				if(h_errno == TRY_AGAIN) {
					logwarn("temporary failure resolving hostname %s", 
						opts->ipaddr); 
				} else {
					break;
				}
				sleep(CGPSCLT_RESOLVE_TIMEOUT);
			}
		}
		if(!ent) {
			die("failed resolve hostname %s", opts->ipaddr);
		} 
		
		if(ent->h_addrtype == AF_INET) {
			struct sockaddr_in sockaddr;

			opts->ipsock = socket(PF_INET, SOCK_STREAM, 0);
			if(opts->ipsock < 0) {
				die("failed create TCP socket");
			}
			debug("created TCP socket");
			
			sockaddr.sin_family = AF_INET;
			sockaddr.sin_port = htons(opts->port);
			memcpy(ent->h_addr, (char *)&sockaddr.sin_addr, sizeof(struct in_addr));
			if(connect(opts->ipsock, 
				   (const struct sockaddr *)&sockaddr,
				   sizeof(struct sockaddr_in)) < 0) {
#ifdef HAVE_INET_NTOA
				die("failed connect to %s on port %d (%s)", 
				    inet_ntoa(sockaddr.sin_addr), opts->port, ent->h_name);
#else
				die("failed connect to %s on port %d", 
				    opts->ipaddr, opts->port);
#endif /* ! HAVE_INET_NTOA */
			}
			debug("connected to %s on port %d", opts->ipaddr, opts->port);
		} else {
			die("host %s has no IPv4 address", opts->ipaddr);
		}
	}
	if(opts->unaddr) {
		struct sockaddr_un sockaddr;
		
		opts->unsock = socket(PF_UNIX, SOCK_STREAM, 0);
		if(opts->unsock < 0) {
			die("failed create UNIX socket");
		}
		debug("created UNIX socket");
		
		sockaddr.sun_family = AF_UNIX;
		strncpy(sockaddr.sun_path, opts->unaddr, sizeof(sockaddr.sun_path));
		if(connect(opts->unsock, 
			   (const struct sockaddr *)&sockaddr,
			   sizeof(struct sockaddr_un)) < 0) {
			die("failed connect to %s", opts->unaddr);
		}
		debug("connected to %s", opts->unaddr);
	}
}
