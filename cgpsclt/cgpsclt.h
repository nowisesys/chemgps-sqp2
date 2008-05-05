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

#ifndef __CGPSCLT_H__
#define __CGPSCLT_H__

int init_socket(struct options *opts);
int request(struct options *opts, struct client *peer);

#define CGPSCLT_CONN_FAILED -1    /* permanent connection error */
#define CGPSCLT_CONN_SUCCESS 0    /* successful connected */
#define CGPSCLT_CONN_RETRY   1    /* temporary connection error (retry) */

#if ! defined(CGPSCLT_EXTERN)

# define CGPSCLT_RETRY_LIMIT 5    /* number of connect retries */
# define CGPSCLT_RETRY_SLEEP 6    /* timeout between retries */

void parse_options(int argc, char **argv, struct options *opts);

#endif

#endif /* __CGPSCLT_H__ */
