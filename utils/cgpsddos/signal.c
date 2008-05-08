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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#include <signal.h>
#include <errno.h>

#include "cgpsddos.h"
#include "cgpssqp.h"

/*
 * The signal handler used by the daemon. We just ignore SIGPIPE and 
 * check if errno is ESPIPE after a write.
 */
static void signal_handler(int sig)
{	
	switch(sig) {
	case SIGTERM:
		if(!opts->quiet) {
			loginfo("received the terminate signal (SIGTERM)");
		}
		opts->state |= CGPSDDOS_STATE_QUIT;
		break;
	case SIGINT:
	case SIGQUIT:
		if(!opts->quiet) {
			loginfo("received the interupt signal from keyboard (SIGINT)");
		}
		opts->state |= CGPSDDOS_STATE_QUIT;
		break;
	case SIGPIPE:
		if(opts->debug && !opts->quiet) {
			debug("received the broken pipe signal (SIGPIPE): write to pipe with no readers");
		}
		break;
	default:
		logerr(0, "received unexpected signal %d", sig);   /* impossible */
		break;
	}
}

/*
 * Setup signal handling. Block all signals except thoose handled
 * in the signal handling function.
 */
void setup_signals(struct options *popt)
{
	sigset_t mask;
	
	popt->newact = malloc(sizeof(struct sigaction));
	if(!popt->newact) {
		die("failed alloc memory");
	}
	memset(popt->newact, 0, sizeof(struct sigaction));
	
	popt->oldact = malloc(sizeof(struct sigaction));
	if(!popt->oldact) {
		die("failed alloc memory");
	}
	memset(popt->oldact, 0, sizeof(struct sigaction));
	
	popt->newact->sa_handler = signal_handler;
	
	if(sigfillset(&mask) < 0) {
		die("failed call sigfillset");
	}
	if(sigdelset(&mask, SIGTERM) < 0) {
		die("failed call sigdelset (SIGTERM)");
	}
	if(sigdelset(&mask, SIGPIPE) < 0) {
		die("failed call sigdelset (SIGPIPE)");
	}
	
	debug("unblocking keyboard interrupt signal (SIGINT)"); 
	if(sigdelset(&mask, SIGINT) < 0) {
		die("failed call sigdelset (SIGINT)");
	}
	if(sigdelset(&mask, SIGQUIT) < 0) {
		die("failed call sigdelset (SIGQUIT)");
	}
	
	if(sigprocmask(SIG_SETMASK, &mask, NULL) < 0) {
		die("failed call sigprocmask");
	}
		
	if(sigaction(SIGTERM, popt->newact, NULL) != 0) {
		logerr("failed setup signal action (SIGTERM)");
	}
	else {
		debug("installed signal handler for SIGTERM");
	}

	if(sigaction(SIGINT, popt->newact, NULL) != 0) {
		logerr("failed setup signal action (SIGINT)");
	}
	else {
		debug("installed signal handler for SIGINT");
	}

	if(sigaction(SIGQUIT, popt->newact, NULL) != 0) {
		logerr("failed setup signal action (SIGQUIT)");
	}
	else {
		debug("installed signal handler for SIGQUIT");
	}

	if(sigaction(SIGPIPE, popt->newact, NULL) != 0) {
		logerr("failed setup signal action (SIGPIPE)");
	}
	else {
		debug("installed signal handler for SIGPIPE");
	}
}

/*
 * Restore signal handling.
 */
void restore_signals(struct options *popt)
{	
	if(sigaction(SIGTERM, popt->oldact, NULL) != 0) {
		logerr("failed restore signal action (SIGTERM)");
	}
	else {
		debug("restored signal handler for SIGTERM");
	}

	if(sigaction(SIGINT, popt->oldact, NULL) != 0) {
		logerr("failed restore signal action (SIGINT)");
	}
	else {
		debug("restored signal handler for SIGINT");
	}

	if(sigaction(SIGQUIT, popt->oldact, NULL) != 0) {
		logerr("failed restore signal action (SIGQUIT)");
	}
	else {
		debug("restored signal handler for SIGQUIT");
	}

	if(sigaction(SIGPIPE, popt->oldact, NULL) != 0) {
		logerr("failed restore signal action (SIGPIPE)");
	}
	else {
		debug("restored signal handler for SIGPIPE");
	}
	
	if(popt->newact) {
		free(popt->newact);
		popt->newact = NULL;
	}
	if(popt->oldact) {
		free(popt->oldact);
		popt->oldact = NULL;
	}
}
