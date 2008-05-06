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
#include <libgen.h>
#include <chemgps.h>

#include "cgpssqp.h"
#include "cgpsddos.h"

struct cgpsddos *ddos;
struct options *opts;

#ifdef HAVE_ATEXIT
void exit_handler(void)
{
	if(ddos) {
		debug("cleaning up at exit...");
		if(ddos->opts) {
			if(ddos->opts->cgps) {
				free(ddos->opts->cgps);
			}
			if(ddos->opts->ipaddr) {
				if(ddos->opts->ipaddr == CGPSD_DEFAULT_ADDR) {
					free(ddos->opts->ipaddr);
				}
			}
			if(ddos->opts->data) {
				free(ddos->opts->data);
			}
			if(ddos->opts->unaddr) {
				if(ddos->opts->unaddr != CGPSD_DEFAULT_SOCK) {
					free(ddos->opts->unaddr);
				}
			}
		}
		if(ddos->accept) {
			free(ddos->accept);
		}
		if(ddos->slaves) {
			free(ddos->slaves);
		}
		free(ddos);
		ddos = NULL;
	}
}
#endif /* HAVE_ATEXIT */

int main(int argc, char **argv)
{
        opts = malloc(sizeof(struct options));
	if(!opts) {
		die("failed alloc memory");
	}
	memset(opts, 0, sizeof(struct options));	
	
	ddos = malloc(sizeof(struct cgpsddos));
	if(!ddos) {
		die("failed alloc memory");
	}
	memset(ddos, 0, sizeof(struct cgpsddos));
	ddos->opts = opts;
	
        opts->cgps = malloc(sizeof(struct cgps_options));
	if(!opts->cgps) {
		die("failed alloc memory");
	}
	memset(opts->cgps, 0, sizeof(struct cgps_options));
	
	ddos->mode = CGPSDDOS_LOCAL;	
	opts->prog = basename(argv[0]);
	opts->parent = getpid();
	
#ifdef HAVE_ATEXIT
	if(atexit(exit_handler) != 0) {
		logerr("failed register main exit handler");
	}
#endif
	
	parse_options(argc, argv, ddos);

	setup_signals(opts);
	switch(ddos->mode) {
	case CGPSDDOS_MASTER:
		run_master(ddos);
		break;
	case CGPSDDOS_SLAVE:
		run_slave(ddos);
		break;
	case CGPSDDOS_LOCAL:
		run_local(ddos);
		break;
	default:
		die("unknown mode %d", ddos->mode);
	}
	restore_signals(opts);
	
	return 0;
}
