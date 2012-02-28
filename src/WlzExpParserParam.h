#ifndef WLZ_PARSERPARAM_H
#define WLZ_PARSERPARAM_H
#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzExpParserParam_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzExpParserParam.h
* \author       Bill Hill
* \date         October 2011
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               MRC Institute of Genetics and Molecular Medicine,
*               University of Edinburgh,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C), [2012],
* The University Court of the University of Edinburgh,
* Old College, Edinburgh, UK.
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be
* useful but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public
* License along with this program; if not, write to the Free
* Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA  02110-1301, USA.
* \brief	Type definitions for the flex lexical analyser in
* 		parsing Woolz expressions for the Woolz IIP server.
* \ingroup	WlzIIPServer
*/

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef YY_NO_UNISTD_H
#define YY_NO_UNISTD_H 1
#endif /* YY_NO_UNISTD_H */

#include "WlzExpTypeParser.h"
#include "WlzExpLexer.h"
#include "WlzExpression.h"

typedef struct _WlzExpParserParam
{
  yyscan_t 	scanner;
  WlzExp       	*exp;
  unsigned int	nPar;
  unsigned int 	par[4];
}WlzExpParserParam;

#define YYPARSE_PARAM data
#define YYLEX_PARAM   ((WlzExpParserParam*)data)->scanner

extern	int	yyparse(void *);

#ifdef __cplusplus
}
#endif

#endif /* WLZ_PARSERPARAM_H */
