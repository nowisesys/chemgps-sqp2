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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#include "cgpsddos.h"
#include "cgpssqp.h"

#ifndef NDEBUG
static void print_sockaddr(const struct sockaddr *sockaddr, socklen_t addrlen, int sent)
{
	char port[NI_MAXSERV + 1];
	char host[NI_MAXHOST + 1];
	if(getnameinfo(sockaddr, addrlen, 
		       host, NI_MAXHOST, 
		       port, NI_MAXSERV, NI_NUMERICSERV) == 0) {
		if(sent) {
			debug("successful sent message to peer %s on port %s", host, port);
		} else {
			debug("successful received message from peer %s on port %s", host, port);
		}
	}
}
#endif

/*
 * Send datagram message on (unconnected) socket sock. The destination host is
 * defined by the sockaddr argument.
 */
ssize_t send_dgram(int sock, const char *buff, size_t size, const struct sockaddr *sockaddr, socklen_t addrlen)
{
	ssize_t bytes;

	debug("sending message on socket %d", sock);
	
	if(sockaddr) {
		debug("sending message on unconnected socket %d", sock);
		bytes = sendto(sock, buff, size, 0, sockaddr, addrlen);
		if(bytes == size) {
#ifndef NDEBUG
			print_sockaddr(sockaddr, addrlen, 1);
#endif
		}
	} else {
		debug("sending message on connected socket %d", sock);
		bytes = send(sock, buff, size, 0);
		if(bytes == size) {
#ifndef NDEBUG
			debug("successful sent message to peer");
#endif
		}
	}
	if(bytes < size) {
		logerr("failed send message to peer");
	}
	return bytes;
}

/*
 * Receive datagram message from (unconnected) socket. The peer address is 
 * stored in the sockaddr argument of length socklen or should be NULL if 
 * the socket is connected.
 */
ssize_t receive_dgram(int sock, char *buff, size_t size, struct sockaddr *sockaddr, socklen_t *addrlen)
{
	ssize_t bytes;
	
	debug("receiving message on socket %d", sock);
	
	if(sockaddr) {
		bytes = recvfrom(sock, buff, size, 0, sockaddr, addrlen);
		if(bytes > 0) {
#ifndef NDEBUG
			print_sockaddr(sockaddr, *addrlen, 0);
#endif
		} 
	} else {
		bytes = recv(sock, buff, size, 0);
		if(bytes > 0) {
#ifndef NDEBUG
			debug("successful received message from peer");
#endif
		}
	}
	if(bytes > 0 && bytes < size) {
		buff[bytes] = '\0';
	}
	return bytes;
}
