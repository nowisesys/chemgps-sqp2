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
#include <chemgps.h>

/*
 *  * The lookup table of all prediction results.
 *  */
const struct cgps_result_entry cgps_result_entry_list[] = {
	{ PREDICTED_CONTRIB_SSW, "cssw", "Predicted Contribution SSW" },
	{ PREDICTED_CONTRIB_SSW_GROUP, "csswgrp", "Predicted Contribution SSW Group" },
	{ PREDICTED_CONTRIB_SMW, "csmw", "Predicted Contribution SMW" },
	{ PREDICTED_CONTRIB_SMW_GROUP, "csmwgrp", "Predicted Contribution SMW Group" },
	{ PREDICTED_CONTRIB_DMOD_X, "cdmodx", "Predicted DModX Contribution" },
	{ PREDICTED_CONTRIB_DMOD_X_GROUP, "cdmodxgrp", "Predicted DModX Contribution Group" },
	
	{ PREDICTED_DMOD_X_PS, "dmodxps", "Predicted DModXPS" },
	{ PREDICTED_DMOD_X_PS_COMB, "dmodxpscomb", "Predicted DModXCombinedPS" },
	{ PREDICTED_PMOD_X_PS, "pmodxps", "Predicted PModXPS" },
	{ PREDICTED_PMOD_X_COMB_PS, "pmodxcombps", "Predicted PModXCombinedPS" },
	
	{ PREDICTED_TPS, "tps", "Predicted TPS" },
	{ PREDICTED_TCV_PS, "tcvps", "Predicted TcvPS" },
	{ PREDICTED_TCV_SEPS, "tcvseps", "Predicted TcvSEPS" },
	{ PREDICTED_TCV_SED_FPS, "tcvsedfps", "Predicted TcvSEDFPS" },
	
	{ PREDICTED_T2_RANGE_PS, "t2rangeps", "Predicted T2RangePS" },
	{ PREDICTED_X_OBS_RES_PS, "xobsresps", "Predicted XObsResPS" },
	{ PREDICTED_X_OBS_PRED_PS, "xobspredps", "Predicted XObsPredPS" },
	{ PREDICTED_X_VAR_PS, "xvarps", "Predicted XVarPS" },
	{ PREDICTED_X_VAR_RES_PS, "xvarresps", "Predicted XVarResPS" },
	
	{ PREDICTED_SERR_LPS, "serrlps", "Predicted SerrLPS" },
	{ PREDICTED_SERR_UPS, "serrups", "Predicted SerrUPS" },
	
	{ PREDICTED_Y_PRED_PS, "ypredps", "Predicted YPredPS" },
	{ PREDICTED_Y_PRED_CV_CONF_INT_PS, "ypredcvconfintps", "Predicted YPredCVConfIntPS" },
	{ PREDICTED_Y_CV_PS, "ycvps", "Predicted YcvPS" },
	{ PREDICTED_Y_CV_SEPS, "ycvseps", "Predicted YcvSEPS" },
	{ PREDICTED_Y_OBS_RES_PS, "yobsresps", "Predicted YObsResPS" },
	{ PREDICTED_Y_VAR_PS, "yvarps", "Predicted YVarPS" },
	{ PREDICTED_Y_VAR_RES_PS, "yvarresps", "Predicted YVarResPS" },
	
	{ PREDICTED_RESULTS_ALL, "all", "Output All Predicted Results" },
	{ PREDICTED_RESULTS_NONE, NULL, NULL },
	{ PREDICTED_RESULTS_LAST, NULL, NULL }
};

/*
 *  * Lookup predicted result entry by name.
 *  */
const struct cgps_result_entry * cgps_result_entry_value(const char *name)
{
	const struct cgps_result_entry *entry;
	for(entry = cgps_result_entry_list; entry->name; ++entry) {
		if(strcmp(entry->name, name) == 0) {
			return entry;
		}
	}
	return NULL;
}

/*
 *  * Lookup predicted result entry by value.
 *  */
const struct cgps_result_entry * cgps_result_entry_name(int value)
{
	const struct cgps_result_entry *entry;
	for(entry = cgps_result_entry_list; entry->name; ++entry) {
		if(entry->value == value) {
			return entry;
		}
	}
	return NULL;
}
