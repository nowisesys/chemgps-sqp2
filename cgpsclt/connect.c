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

#include "cgpssqp.h"
#include "cgpsclt.h"

/*
 * Open connecttion to server and make request. Retry on temporary failure. If
 * retry limit is reached, return -1 and let caller decide to sleep and retry
 * again or permanent fail.
 */
int client_connect(struct options *popt)
{
	struct client peer;
	int retry;
	
	for(retry = 0; retry < CGPSCLT_RETRY_LIMIT; ++retry) {
		if(retry != 0) {
			if(popt->verbose && !popt->quiet) {
				logwarn("retry attempt %d/%d (connect)", retry, CGPSCLT_RETRY_LIMIT);
			}
		}
		switch(init_socket(popt)) {
		case CGPSCLT_CONN_FAILED:
			return -1;
			break;
		case CGPSCLT_CONN_RETRY:
			if(popt->verbose && !popt->quiet) {
				logwarn("server busy, waiting %d seconds before retrying (connect)", CGPSCLT_RETRY_SLEEP);
			}
			sleep(CGPSCLT_RETRY_SLEEP);
			continue;
		case CGPSCLT_CONN_SUCCESS:
			if(retry != 0) {
				if(popt->verbose && !popt->quiet) {
					loginfo("success after %d attempts (connect)", retry);
				}
			}
			retry = 0;
			break;
		}
		if(retry == 0) {
			break;
		}
	}
	if(retry == CGPSCLT_RETRY_LIMIT) {
		if(popt->verbose && !popt->quiet) {
			logerr("failed connect after %d seconds (%d retries)",
			       CGPSCLT_RETRY_SLEEP * CGPSCLT_RETRY_LIMIT, CGPSCLT_RETRY_LIMIT);
		}
		return -1;
	}
	
	peer.sock = popt->unsock ? popt->unsock : popt->ipsock;
	peer.opts = popt;
	
	for(retry = 0; retry < CGPSCLT_RETRY_LIMIT; ++retry) {
		if(retry != 0) {
			if(popt->verbose && !popt->quiet) {
				logwarn("retry attempt %d/%d (request)", retry, CGPSCLT_RETRY_LIMIT);
			}
		}
		switch(request(popt, &peer)) {
		case CGPSCLT_CONN_FAILED:
			return -1;
			break;
		case CGPSCLT_CONN_RETRY:
			if(popt->verbose && !popt->quiet) {
				logwarn("server busy, waiting %d seconds before retrying (request)", CGPSCLT_RETRY_SLEEP);
			}
			sleep(CGPSCLT_RETRY_SLEEP);
			continue;
		case CGPSCLT_CONN_SUCCESS:
			if(retry != 0) {
				if(popt->verbose && !popt->quiet) {
					loginfo("successful after %d attempts (request)", retry);
				}
			}
			retry = 0;
			break;
		}
		if(retry == 0) {
			break;
		}
	}
	if(retry == CGPSCLT_RETRY_LIMIT) {
		if(popt->verbose && !popt->quiet) {
			logerr("failed request after %d seconds (%d retries)",
			       CGPSCLT_RETRY_SLEEP * CGPSCLT_RETRY_LIMIT, CGPSCLT_RETRY_LIMIT);
		}
		return -1;
	}
	
	return 0;
}
