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

%token		TOKEN_BACKGROUND
%token		TOKEN_DIFF
%token		TOKEN_DILATION
%token		TOKEN_DOMAIN
%token		TOKEN_EROSION
%token		TOKEN_FILL
%token		TOKEN_INTERSECT
%token		TOKEN_OCCUPANCY
%token		TOKEN_SETVALUE
%token		TOKEN_THRESHOLD
%token		TOKEN_TRANSFER
%token		TOKEN_UNION
 
%token <u> 	TOKEN_UINT
%token <cmp> 	TOKEN_CMP
 
%left 		TOKEN_SEP
%right 		TOKEN_DASH

%type <idx_rng> idx_rng
%type <idx_lst> idx_lst
%type <exp> 	exp
 
%%
 
input: 
        	exp
		{ ((WlzExpParserParam*)data)->exp    = $1;
		  ((WlzExpParserParam*)data)->nPar   = 0;
		} |
		exp TOKEN_SEP TOKEN_UINT TOKEN_SEP TOKEN_UINT
		{ ((WlzExpParserParam*)data)->exp = $1;
		  ((WlzExpParserParam*)data)->nPar = 2;
		  ((WlzExpParserParam*)data)->par[0] = $3;
		  ((WlzExpParserParam*)data)->par[1] = $5;
		} |
		exp TOKEN_SEP TOKEN_UINT TOKEN_SEP TOKEN_UINT
		TOKEN_SEP TOKEN_UINT
		{ ((WlzExpParserParam*)data)->exp = $1;
		  ((WlzExpParserParam*)data)->nPar = 3;
		  ((WlzExpParserParam*)data)->par[0] = $3;
		  ((WlzExpParserParam*)data)->par[1] = $5;
		  ((WlzExpParserParam*)data)->par[2] = $7;
		} |
		exp TOKEN_SEP TOKEN_UINT TOKEN_SEP TOKEN_UINT
		TOKEN_SEP TOKEN_UINT TOKEN_SEP TOKEN_UINT
		{ ((WlzExpParserParam*)data)->exp = $1;
		  ((WlzExpParserParam*)data)->nPar = 4;
		  ((WlzExpParserParam*)data)->par[0] = $3;
		  ((WlzExpParserParam*)data)->par[1] = $5;
		  ((WlzExpParserParam*)data)->par[2] = $7;
		  ((WlzExpParserParam*)data)->par[3] = $9;
		}
        	;
 
idx_rng:	TOKEN_UINT TOKEN_DASH TOKEN_UINT
		{
		  $$ = WlzExpMakeIndexRange($1, $3);
		}
		;

idx_lst:	TOKEN_UINT
		{
		  $$ = WlzExpMakeIndex($1);
		} |
		idx_rng
		{
		  $$ = $1;
		} |
		idx_lst TOKEN_SEP idx_lst
		{
		  $$ = WlzExpMakeIndexList($1, $3);
		}
		;

exp:
		TOKEN_UINT
		{
		  $$ = WlzExpMakeIndex($1);
		} |
		TOKEN_BACKGROUND TOKEN_OP exp TOKEN_SEP TOKEN_UINT TOKEN_CP
		{
		  $$ = WlzExpMakeBackground($3, $5);
		} |
		TOKEN_DIFF TOKEN_OP exp TOKEN_SEP exp TOKEN_CP
		{
		  $$ = WlzExpMakeDiff($3, $5);
		} |
		TOKEN_DILATION TOKEN_OP exp TOKEN_SEP TOKEN_UINT TOKEN_CP
		{
		  $$ = WlzExpMakeDilation($3, $5);
		} |
		TOKEN_DOMAIN TOKEN_OP exp TOKEN_CP
		{
		  $$ = WlzExpMakeDomain($3);
		} |
		TOKEN_EROSION TOKEN_OP exp TOKEN_SEP TOKEN_UINT TOKEN_CP
		{
		  $$ = WlzExpMakeErosion($3, $5);
		} |
		TOKEN_FILL TOKEN_OP exp TOKEN_CP
		{
		  $$ = WlzExpMakeFill($3);
		} |
		TOKEN_INTERSECT TOKEN_OP exp TOKEN_SEP exp TOKEN_CP
		{
		  $$ = WlzExpMakeIntersect($3, $5);
		} |
		TOKEN_OCCUPANCY TOKEN_OP TOKEN_CP
		{
		  $$ = WlzExpMakeOccupancy(NULL);
		} |
		TOKEN_OCCUPANCY TOKEN_OP exp TOKEN_CP
		{
		  $$ = WlzExpMakeOccupancy($3);
		} |
		TOKEN_OCCUPANCY TOKEN_OP idx_lst TOKEN_CP
		{
		$$ = WlzExpMakeOccupancy($3);
		} |
		TOKEN_SETVALUE TOKEN_OP exp TOKEN_SEP
		TOKEN_UINT TOKEN_CP
		{
		  $$ = WlzExpMakeSetvalue($3, $5);
		} |
		TOKEN_THRESHOLD TOKEN_OP exp TOKEN_SEP
		TOKEN_UINT TOKEN_SEP TOKEN_CMP TOKEN_CP
		{
		  $$ = WlzExpMakeThreshold($3, $5, $7);
		} |
		TOKEN_TRANSFER TOKEN_OP exp TOKEN_SEP exp TOKEN_CP
		{
		  $$ = WlzExpMakeTransfer($3, $5);
		} |
		TOKEN_UNION TOKEN_OP exp TOKEN_SEP exp TOKEN_CP
		{
		  $$ = WlzExpMakeUnion($3, $5);
		} |
		TOKEN_UNION TOKEN_OP exp TOKEN_SEP idx_lst TOKEN_CP
		{
		  $$ = WlzExpMakeUnion($3, $5);
		} |
		TOKEN_UNION TOKEN_OP idx_lst TOKEN_SEP exp TOKEN_CP
		{
		  $$ = WlzExpMakeUnion($3, $5);
		} |
		TOKEN_UNION TOKEN_OP idx_lst TOKEN_CP
		{
		  $$ = WlzExpMakeUnion($3, NULL);
		};
 
%%
