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

#ifndef __CGPSCLT_H__
#define __CGPSCLT_H__

int init_socket(struct options *popt);
int request(struct options *popt, struct client *peer);
int client_connect(struct options *popt);

#define CGPSCLT_CONN_FAILED -1    /* permanent connection error */
#define CGPSCLT_CONN_SUCCESS 0    /* successful connected */
#define CGPSCLT_CONN_RETRY   1    /* temporary connection error (retry) */

#if ! defined(CGPSCLT_EXTERN)

# define CGPSCLT_RETRY_LIMIT 12   /* number of connect/request retries */
# define CGPSCLT_RETRY_SLEEP 5    /* timeout between retries */

# define CGPSCLT_LOOP_COUNT  3    /* number of retry loops */
# define CGPSCLT_LOOP_SLEEP  60   /* sleep between retry loop iterations */

# define CGPSCLT_RETRY_TOTAL CGPSCLT_RETRY_LIMIT * CGPSCLT_RETRY_SLEEP

#endif

#endif /* __CGPSCLT_H__ */
