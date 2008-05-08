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

#ifndef __CGPSD_H__
#define __CGPSD_H__

#define CGPSD_QUEUE_LENGTH 50  /* max length for queue of pending connections */

#define CGPSD_STATE_INITILIZING  0
#define CGPSD_STATE_DAEMONIZED   1
#define CGPSD_STATE_RUNNING      2
#define CGPSD_STATE_CLOSING      4
#define CGPSD_STATE_RELOAD       8

#define cgpsd_done(state) (((state) & CGPSD_STATE_CLOSING))

void service(struct options *popt);
void * process_request(void *peer);
int init_socket(struct options *popt);
void close_socket(struct options *popt);
void setup_signals(struct options *popt);
void restore_signals(struct options *popt);

#endif /* __CGPSD_H__ */
