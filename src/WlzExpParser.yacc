%{

#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
static char _WlzExpParser_yacc[] = "MRC HGU $Id$";
#endif
/*!
* \file         WlzExpParser.yacc
* \author       Bill Hill
* \date         October 2011
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C) 2011 Medical research Council, UK.
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
* \brief        Parser for use with bison (may not be compatible
*               with yacc) in building Woolz expressions for the Woolz
*		IIP server.
* \note         This file has the suffix ".yacc" to try and work around
*               automake and autoconf daftness.
* \ingroup      WlzIIPServer
*/

#include "WlzExpTypeParser.h"
#include "WlzExpParserParam.h"

%}
 
%define api.pure

%token		TOKEN_OP
%token		TOKEN_CP
%token		TOKEN_SEP
%token		TOKEN_DASH
%token		TOKEN_INTERSECT
%token		TOKEN_UNION
%token		TOKEN_DILATION
%token		TOKEN_EROSION
%token		TOKEN_DIFF
%token		TOKEN_THRESHOLD
 
%token <u> 	TOKEN_UINT
%token <cmp> 	TOKEN_CMP
 
%type <exp> 	exp
 
%%
 
input: 
        	exp
		{ ((WlzExpParserParam*)data)->exp    = $1;
		  ((WlzExpParserParam*)data)->nPar   = 0;
		} |
        	exp TOKEN_SEP TOKEN_UINT
		{ ((WlzExpParserParam*)data)->exp    = $1;
		  ((WlzExpParserParam*)data)->nPar   = 1;
		  ((WlzExpParserParam*)data)->par[0] = $3;
		} |
        	exp TOKEN_SEP TOKEN_UINT TOKEN_SEP TOKEN_UINT
		{ ((WlzExpParserParam*)data)->exp    = $1;
		  ((WlzExpParserParam*)data)->nPar   = 2;
		  ((WlzExpParserParam*)data)->par[0] = $3;
		  ((WlzExpParserParam*)data)->par[1] = $5;
		} |
        	exp TOKEN_SEP TOKEN_UINT TOKEN_SEP TOKEN_UINT
		    TOKEN_SEP TOKEN_UINT
		{ ((WlzExpParserParam*)data)->exp    = $1;
		  ((WlzExpParserParam*)data)->nPar   = 3;
		  ((WlzExpParserParam*)data)->par[0] = $3;
		  ((WlzExpParserParam*)data)->par[1] = $5;
		  ((WlzExpParserParam*)data)->par[2] = $7;
		} |
        	exp TOKEN_SEP TOKEN_UINT TOKEN_SEP TOKEN_UINT
		    TOKEN_SEP TOKEN_UINT TOKEN_SEP TOKEN_UINT
		{ ((WlzExpParserParam*)data)->exp    = $1;
		  ((WlzExpParserParam*)data)->nPar   = 4;
		  ((WlzExpParserParam*)data)->par[0] = $3;
		  ((WlzExpParserParam*)data)->par[1] = $5;
		  ((WlzExpParserParam*)data)->par[2] = $7;
		  ((WlzExpParserParam*)data)->par[3] = $9;
		}
        	;
 
exp:
		TOKEN_UINT
		{ $$ = WlzExpMakeIndex($1, $1); } |
		TOKEN_UINT TOKEN_DASH TOKEN_UINT
		{ $$ = WlzExpMakeIndex($1, $3); } |
		TOKEN_OP exp TOKEN_CP
		{ $$ = $2 } |
		TOKEN_THRESHOLD TOKEN_OP exp TOKEN_SEP
		TOKEN_UINT TOKEN_SEP TOKEN_CMP TOKEN_CP
		{ $$ = WlzExpMakeThreshold($3, $5, $7); } |
		TOKEN_DIFF TOKEN_OP exp TOKEN_SEP exp TOKEN_CP
		{ $$ = WlzExpMakeDiff($3, $5); } |
		TOKEN_EROSION TOKEN_OP exp TOKEN_SEP TOKEN_UINT TOKEN_CP
		{ $$ = WlzExpMakeErosion($3, $5); } |
		TOKEN_DILATION TOKEN_OP exp TOKEN_SEP TOKEN_UINT TOKEN_CP
		{ $$ = WlzExpMakeDilation($3, $5); } |
		TOKEN_UNION TOKEN_OP exp TOKEN_SEP exp TOKEN_CP
		{ $$ = WlzExpMakeUnion($3, $5); } |
		TOKEN_INTERSECT TOKEN_OP exp TOKEN_SEP exp TOKEN_CP
		{ $$ = WlzExpMakeIntersect($3, $5); }
		;
 
%%

