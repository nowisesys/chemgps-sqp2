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

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>

#define CGPS_CHECK_FLOAT_MATRIX 0
#define CGPS_CHECK_STRING_MATRIX 1

#include "cgpssqp.h"

extern const char * cgps_simcaq_error(void);

/*
 * Load data for prediction.
 */

/*
 * Dynamic detect number of observations from input data. The n value is used
 * to prevent counting lines without values.
 */
static int cgps_predict_count_observations(const char *file, int *numobs)
{
	FILE *fs;
	int c, n = 0;
	
	fs = fopen(file, "r");
	if(!fs) {
		logerr("failed open file %s for reading", file);
		return -1;
	}
	
	*numobs = 0;
	while((c = getc(fs)) != EOF) {
		if(isdigit(c)) {
			n = 1;
		}
		if(n == 1 && c == '\n') {
			(*numobs)++;
			n = 0;
		}
	}
	fclose(fs);
	return 0;
}

/*
 * Returns next descriptor from input buffer starting at offset. The
 * offset parameter is modified to point to the next descriptor. NULL
 * is returned when no more descriptors exists.
 */
static const char * cgps_predict_next_decriptor(const char *buff, size_t *offset, size_t *length)
{
	size_t size = 0;                   /* field size (float number) */
	size_t next = 0;                   /* number of whitespace chars */
	const char *delim = " ,:;\t\n";    /* field delimiter chars */
	
	/* 
	 * Guess values are separated by either: space (' '), comma (','), 
	 * colon (':'), semi-colon (';') or tabs ('\t'). Pretty much all 
	 * spreadsheet applications uses either one of these as separator
	 * character when exporting data.
	 */

	if(opts->verbose > 2) {
		debug("parsing descriptor: offset %d", *offset);
	}
	
	buff += *offset;
	size = strcspn(buff, delim);
	if(size) {
		next = strspn(buff + size, delim);
		*offset += size + next;
		*length  = size;
	}
	
	if(opts->verbose > 2) {
		debug("parsing descriptor: size = %d (used), next = %d (skipped)", size, next);
	}
	if(!size) {
		return NULL;
	}
	if(!next) {
		if(buff[0] == '\n' || buff[0] == '\0') {
			return NULL;
		}
	}
	return buff;
}

/*
 * Initilize reorder table. By default, all input data is just a matrix
 * of floating point numbers, so we got a one-to-one mapping.
 */
static void cgps_predict_init_reorder_table(int *reorder, int size, int offset)
{
	int i;

	debug("initilizing descriptor reorder table");
	for(i = 0; i < size; ++i) {
		reorder[i + offset] = i;
		if(offset) {
			debug("reorder column %d -> %d in indata", i + offset, i);
		}
	}
}

/*
 * Update reorder table. This function should be called when its detected 
 * that input data (raw) contains a header of quantitative variable names 
 * (in buff) that should be matched against the projects list of quantitative 
 * variable names (in names).
 */
static int cgps_predict_update_reorder_table(int *reorder, int size, SQX_StringVector *names, char *buff)
{
	const char *str, *pp;
	int i, j, num;
	
	/*
	 * By setting all entries to -1 we ensure that values no used by the
	 * project is identified as unusable (-1) by default.
	 */
	for(i = 0; i < size; ++i) {
		reorder[i] = -1;
	}
	
	num = SQX_GetNumStringsInVector(names);
	for(i = 0; i < num; ++i) {
		if(!SQX_GetStringFromVector(names, i + 1, &str)) {
			logerr("failed get string from vector (%s)", cgps_simcaq_error());
			return -1;
		}
		if(str) {
			size_t offset = 0, length = 0;
			j = 0;
			while((pp = cgps_predict_next_decriptor(buff, &offset, &length))) {
				if((strlen(str) == length) && strncmp(str, pp, length) == 0) {
					reorder[j] = i;
				}
				++j;
			}
		}
	}	
	if(opts->debug && opts->verbose) {
		for(i = 0; i < size; ++i) {
			if(reorder[i] == -1) {
				debug("reorder: mapping position %d -> %d (ignored)", i, reorder[i]);
			} else if(reorder[i] != i) {
				SQX_GetStringFromVector(names, reorder[i] + 1, &str);
				debug("reorder: mapping position %d -> %d (descriptor %s)", i, reorder[i], str);
			} else {
				debug("reorder: mapping position %d -> %d (identity map)", i, reorder[i]);
			}
		}
	}
	return 0;
}

/*
 * Count number of fields in indata (from buffer buff).
 */
static int cgps_predict_indata_count_fields(const char *buff)
{
	int count = 0;
	size_t offset = 0, length = 0;
	const char *pp;
	
	while((pp = cgps_predict_next_decriptor(buff, &offset, &length))) {
		++count;
	}
	return count;
}

/*
 * Return true if buffer contains descriptors header.
 */
static int cgps_predict_indata_has_header(const char *buff)
{
	size_t offset = 0, length = 0;
	const char *pp;
	char *ep;
	
	pp = cgps_predict_next_decriptor(buff, &offset, &length);
	pp = cgps_predict_next_decriptor(buff, &offset, &length);
	if(!pp) {
		return 0;
	}
	
	if(!strtod(pp, &ep)) {
		return pp == ep ? 1 : 0;
	}
	return 0;
}

/*
 * Return true if buffer contains molecule header.
 */
static int cgps_predict_indata_has_molid(const char *buff)
{
	size_t offset = 0, length = 0;
	const char *pp; 
	char *ep;
	
	pp = cgps_predict_next_decriptor(buff, &offset, &length);
	if(!pp) {
		return 0;
	}
	
	if(!strtod(pp, &ep)) {
		return pp == ep ? 1 : 0;
	}
	return 0;
}

/*
 * Scan indata and return a suitable reorder table.
 */
static int * cgps_predict_scan_indata(char *buff, int *reorder, SQX_StringVector *names, int *skip)
{
	int fields;
	int columns = SQX_GetNumStringsInVector(names);
	
	fields  = cgps_predict_indata_count_fields(buff);
	reorder = realloc(reorder, fields * sizeof(int));
	if(!reorder) {
		logerr("failed alloc memory");
		return NULL;
	}
	debug("allocated reorder table with %d entries", fields);
	
	if(cgps_predict_indata_has_header(buff)) {
		debug("detected descriptor header, updating reorder table");
				
		*skip = 1;
		if(cgps_predict_update_reorder_table(reorder, fields, names, buff) < 0) {
			logerr("failed create descriptors reorder table");
			free(reorder);
			return NULL;
		}
	} else if(cgps_predict_indata_has_molid(buff)) {
		if(fields > (columns + 1)) {
			logerr("too many columns in input data (expected: %d, got: %d)",
			       columns + 1, fields);
			free(reorder);
			return NULL;
		}
		debug("deteted molecule id in first field, enable index shift");
		reorder[0] = -1;
		cgps_predict_init_reorder_table(reorder, fields, 1);
	} else {
		if(fields != columns) {
			logerr("number of columns in input data and project don't match");
			free(reorder);
			return NULL;
		}
		debug("no headers detected, treating input data as already ordered");
		cgps_predict_init_reorder_table(reorder, fields, 0);
	}
	
	return reorder;
}

/*
 * Load raw data from file stream. The stream endpoint could be a
 * file, wrapped TCP socket, pipe or stdin.
 */
static int cgps_predict_load_stream(FILE *fs, int rows, int columns, SQX_FloatMatrix *matrix, SQX_StringVector *names)
{
	char *buff = NULL;
	size_t size = 0;
	ssize_t bytes;
	int total = 0;
	int i = 0, j = 0, skip = 0;
	int *reorder = NULL;

	while(((bytes = getline(&buff, &size, fs)) != -1) && i < rows) {
		size_t offset = 0;
		size_t length = 0;
		const char *pp;

		if(!reorder) {
			reorder = cgps_predict_scan_indata(buff, reorder, names, &skip);
			if(!reorder) {
				free(buff);
				return -1;
			}
			if(skip) {
				skip = 0;
				continue;
			}
		}
		
		j = 0;
		while((pp = cgps_predict_next_decriptor(buff, &offset, &length))) {
			if(reorder[j] != -1) {
				if(opts->verbose > 1) {
					debug("saving float value %f from %d -> %d", atof(pp), j, reorder[j]);
				}
				if(!SQX_SetDataInFloatMatrix(matrix, i + 1, reorder[j] + 1, atof(pp))) {
					logerr("failed add float value to matrix (%s)", cgps_simcaq_error());
					free(reorder);
					free(buff);
					return -1;
				}
				++total;
			} else {
				debug("ignored value in column %d from input stream", j);
			}
			++j;
		}
		++i;
		
	}
	debug("loaded %d entries total (%d rows) from input stream to %dx%d matrix", total, i, rows, columns);
	
	if(buff) {
		free(buff);
	}
	if(reorder) {
		free(reorder);
	}
	if(!feof(fs)) {
		struct stat st;
		if(fstat(fileno(fs), &st) < 0) {
			logerr("failed stat input stream");
			return -1;
		}
		if(S_ISREG(st.st_mode)) {
			logerr("premature end of file detected in input stream");
			return -1;
		}
	}
	return 0;
}

/*
 * Load raw data from file. Returns 0 is sucessful and -1
 * on failure.
 */
static int cgps_predict_load_file(const char *path, int rows, int columns, SQX_FloatMatrix *matrix, SQX_StringVector *names)
{
	FILE *fs;
	int result;

	fs = fopen(path, "r");
	if(!fs) {
		logerr("failed open file %s for reading", path);
		return -1;
	}
	result = cgps_predict_load_stream(fs, rows, columns, matrix, names);
	fclose(fs);
	
	return result;
}

/*
 * Get number of observations from socket stream.
 */
static int cgps_predict_get_observations(struct client *loader)
{
	char *buff = NULL;
	size_t size = 0;
	struct request_option req;
	
	read_request(&buff, &size, loader->ss);
	if(split_request_option(buff, &req) == CGPSP_PROTO_LAST) {
		free(buff);
		logerr("failed receive number of observations");
		return -1;
	}
	if(req.symbol != CGPSP_PROTO_LOAD) {
		free(buff);
		logerr("expected load option, got '%s'", req.option);
		return -1;
	}
	loader->opts->numobs = atoi(req.value);
	free(buff);
	
	return 0;
}

/*
 * Check parameters to cgps_predict_load_xxx()
 */
static int cgps_predict_load_check_params(struct cgps_project *proj, struct client *loader, SQX_FloatMatrix *fmx, SQX_StringMatrix *smx, SQX_StringVector *names, int checktype)
{
	int error = 0;
	
	if(checktype == CGPS_CHECK_FLOAT_MATRIX) {
		if(!fmx) {
			logerr("Float matrix not initilized");
			error = 1;
		}
		if(smx) {
			logerr("String matrix already initilized");
			error = 1;
		}
	}
	if(checktype == CGPS_CHECK_STRING_MATRIX) {
		if(fmx) {
			logerr("Float matrix already initilized");
			error = 1;
		}
		if(!smx) {
			logerr("String matrix not initilized");
			error = 1;
		}
	}
	if(!proj->handle) {
		logerr("Invalid project handle");
		error = 1;
	}
	if(!names) {
		logerr("Variable name vector uninitilized");
		error = 1;
	}
	if(error) {
		fprintf(loader->ss, "Error: failed load data\n");
		fflush(loader->ss);
		return -1;		
	}
	return 0;
}

/*
 * Load quantitative data (raw).
 */
static int cgps_predict_load_quant_data(struct cgps_project *proj, struct client *loader, SQX_FloatMatrix *matrix, SQX_StringVector *names)
{
	int num = SQX_GetNumStringsInVector(names);

	if(cgps_predict_load_check_params(proj, loader, matrix, NULL, names, CGPS_CHECK_FLOAT_MATRIX) < 0) {
		logerr("invalid parameters to cgps_predict_load_quant_data().");
		return -1;
	}
	
	if(loader->ss) {
		debug("asking peer to send prediction data (quantitative)");
		fprintf(loader->ss, "Load: quant-data\n");
		fflush(loader->ss);
		if(cgps_predict_get_observations(loader) < 0) {
			logerr("failed get number of observations from peer");
			return -1;
		}
	}
	
	if(!loader->opts->numobs) {
		if(loader->opts->data) {
			if(cgps_predict_count_observations(loader->opts->data, &loader->opts->numobs) < 0) {
				logerr("failed count number of observations in input data");
				return -1;
			} else {
				debug("detected %d number of observations in input data", loader->opts->numobs);
			}
		} else {
			logerr("unknown number of observations in input data, use -i or -n option to fix");
			return -1;
		}
	} else {
		debug("using user defined %d number of observations", loader->opts->numobs);
	}
	if(!SQX_InitFloatMatrix(matrix, loader->opts->numobs, num)) {
		logerr("failed initilize float point matrix (%s)", cgps_simcaq_error());
		return -1;
	}
	if(loader->opts->data) {
		if(cgps_predict_load_file(loader->opts->data, loader->opts->numobs, num, matrix, names) < 0) {
			logerr("failed load raw data from file %s", loader->opts->data);
			return -1;
		}
		debug("successful loaded raw data from %s", loader->opts->data);
	} else {
		if(loader->ss) {		
			if(cgps_predict_load_stream(loader->ss, loader->opts->numobs, num, matrix, names) < 0) {
				logerr("failed load raw data from socket");
				return -1;
			}			
		} else {
			if(!loader->opts->batch) {
				loginfo("waiting for raw data input on stdin (%dx%d):", loader->opts->numobs, num);
			}
			if(cgps_predict_load_stream(stdin, loader->opts->numobs, num, matrix, names) < 0) {
				logerr("failed load raw data from stdin");
				return -1;
			}			
			debug("successful loaded raw data from stdin");
		}
	}
	return 0;
}

/*
 * Load qualitative data.
 */
static int cgps_predict_load_qual_data(struct cgps_project *proj, struct client *loader, SQX_StringMatrix *matrix, SQX_StringVector *names)
{
	if(cgps_predict_load_check_params(proj, loader, NULL, matrix, names, CGPS_CHECK_STRING_MATRIX) < 0) {
		logerr("invalid parameters to cgps_predict_load_qual_data().");
		return -1;
	}

	/*
	 * Not yet implemented due to lack of project using qualitative data.
	 */
	logerr("cgps_predict_load_qual_data: not yet implemeted");
	return -1;
}

/*
 * Load qualitative data that are lagged.
 */
static int cgps_predict_load_qual_data_lagged(struct cgps_project *proj, struct client *loader, SQX_StringMatrix *matrix, SQX_StringVector *names)
{
	if(cgps_predict_load_check_params(proj, loader, NULL, matrix, names, CGPS_CHECK_STRING_MATRIX) < 0) {
		logerr("invalid parameters to cgps_predict_load_qual_data_lagged().");
		return -1;
	}
	
	/*
	 * Not yet implemented due to lack of project using qualitative lagged data.
	 */
	logerr("cgps_predict_load_qual_data_lagged: not yet implemeted");
	return -1;
}

/*
 * Load lagged parents variables.
 */
static int cgps_predict_load_lag_parents_data(struct cgps_project *proj, struct client *loader, SQX_FloatMatrix *matrix, SQX_StringVector *names)
{
	if(cgps_predict_load_check_params(proj, loader, matrix, NULL, names, CGPS_CHECK_FLOAT_MATRIX) < 0) {
		logerr("invalid parameters to cgps_predict_load_quant_data().");
		return -1;
	}
	
	/*
	 * Not yet implemented due to lack of project using lagged parents data.
	 */
	logerr("cgps_predict_load_lag_parents_data: not yet implemeted");
	return -1;
}

/*
 * This function gets called to load data for prediction.
 */
int cgps_predict_data(struct cgps_project *proj, void *params, SQX_FloatMatrix *fmx, SQX_StringMatrix *smx, SQX_StringVector *names, int type)
{
	struct client *loader = (struct client *)params;
	
	if(type == CGPS_GET_QUANTITATIVE_DATA) {
		return cgps_predict_load_quant_data(proj, loader, fmx, names);
	} else if(type == CGPS_GET_QUALITATIVE_DATA) {
		return cgps_predict_load_qual_data(proj, loader, smx, names);
	} else if(type == CGPS_GET_QUAL_LAGGED_DATA) {
		return cgps_predict_load_qual_data_lagged(proj, loader, smx, names);
	} else if(type == CGPS_GET_LAG_PARENTS_DATA) {
		return cgps_predict_load_lag_parents_data(proj, loader, fmx, names);
	} else {
		logerr("unknown type %d passed to data loader", type);
		return -1;
	}
	return 0;   /* keep GCC happy ;-) */
}
