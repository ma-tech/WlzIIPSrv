%{

#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
static char _WlzExpLexer_lex[] = "MRC HGU $Id$";
#endif
/*!
* \file         WlzExpLexer.lex
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
* \brief	Lexical analyser for use with flex (may not be compatible
*		with lex) in building Woolz expressions for the Woolz IIP
*		server.
* \note         This file has the suffix ".lex" to try and work around
*		automake and autoconf daftness.
* \ingroup 	WlzIIPServer
*/

#include "WlzExpTypeParser.h"
#include "WlzExpParser.h"

%}
 
%option reentrant noyywrap never-interactive nounistd
%option bison-bridge
 
OP      	"("
CP      	")"
SEP         	","
DASH         	"-"

BACKGROUND	"background"
DIFF        	"diff"
DILATION    	"dilation"
DOMAIN    	"domain"
EROSION     	"erosion"
FILL        	"fill"
INTERSECT	"intersect"
OCCUPANCY	"occupancy"
SETVALUE   	"setvalue"
THRESHOLD   	"threshold"
TRANSFER    	"transfer"
UNION       	"union"

 
UINT      	[0-9]+
CMP		("lt")|("le")|("eq")|("ge")|("gt")
 
%%
 
{UINT}        	{
                  sscanf(yytext,"%d",&yylval->u);
                  return(TOKEN_UINT);
		}
{CMP}        	{
                  yylval->cmp = WlzExpCmpFromStr(yytext);
		  return(TOKEN_CMP);
		}
{BACKGROUND}    {
                  return(TOKEN_BACKGROUND);
		}
{DIFF}     	{
                  return(TOKEN_DIFF);
		}
{DILATION}     	{
                  return(TOKEN_DILATION);
		}
{DOMAIN}     	{
                  return(TOKEN_DOMAIN);
		}
{EROSION}     	{
                  return(TOKEN_EROSION);
		}
{FILL}     	{
                  return(TOKEN_FILL);
		}
{INTERSECT}     {
                  return(TOKEN_INTERSECT);
		}
{OCCUPANCY}     {
                  return(TOKEN_OCCUPANCY);
		}
{SETVALUE}     {
                  return(TOKEN_SETVALUE);
		}
{THRESHOLD}     {
                  return(TOKEN_THRESHOLD);
		}
{TRANSFER}      {
                  return(TOKEN_TRANSFER);
		}
{UNION}     	{
                  return(TOKEN_UNION);
		}
{OP}        	{
		  return(TOKEN_OP);
		}
{CP}        	{
                  return(TOKEN_CP);
		}
{SEP}        	{
                  return(TOKEN_SEP);
		}
{DASH}        	{
                  return(TOKEN_DASH);
		}
.               {
		}
 
%%
 
int yyerror(const char *msg) { fprintf(stderr,"Error:%s\n",msg); return 0; }

