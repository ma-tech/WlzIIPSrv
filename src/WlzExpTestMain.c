#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzExpTestMain_c[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzExpTestMain.c
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
* \brief	This is a test program for the functions used to parse
* 		Woolz expressions in the Woolz IIP server.
* \ingroup	WlzIIPServer
*/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <Wlz.h>
#include "WlzExpression.h"

int 		main(int argc, char *argv[])
{
  int		option,
  		ok = 1,
		noEval = 0,
  		usage = 0,
		verbose = 0;
  unsigned int	i,
  		nPar;
  char		*expStr,
		*expStr2 = NULL,
  		*inFileStr,
  		*outFileStr;
  FILE		*fP = NULL;
  WlzExp 	*exp = NULL;
  WlzObject	*inObj = NULL,
  		*outObj = NULL;
  unsigned int	par[4];
  const char    *errMsgStr;
  WlzErrorNum	errNum = WLZ_ERR_NONE;
  static char	optList[] = "hnvo:s:";
  static char	fileStrDef[] = "-";
  char 		expStrDef[]="dilation(diff(threshold(0,200,lt),1),5)";


  expStr = expStrDef;
  inFileStr = fileStrDef;
  outFileStr = fileStrDef;
  while((usage == 0) && ((option = getopt(argc, argv, optList)) != EOF))
  {
    switch(option)
    {
      case 'o':
        outFileStr = optarg;
	break;
      case 'n':
        noEval = 1;
	break;
      case 's':
        expStr = optarg;
	break;
      case 'v':
        verbose = 1;
	break;
      case 'h':
      default:
        usage = 1;
	break;
    }
  }
  if((usage == 0) && (optind < argc))
  {
    if((optind + 1) != argc)
    {
      usage = 1;
    }
    else
    {
      inFileStr = argv[optind];
    }
  }
  ok = usage == 0;
  if(ok)
  {
    if(((fP = (strcmp(inFileStr, "-")?
              fopen(inFileStr, "r"): stdin)) == NULL) ||
       ((inObj = WlzAssignObject(WlzReadObj(fP, &errNum), NULL)) == NULL) ||
       (errNum != WLZ_ERR_NONE))
    {
      ok = 0;
      (void )fprintf(stderr,
                     "%s: failed to read input object from file %s\n",
		     *argv, inFileStr);
    }
    if(fP && strcmp(inFileStr, "-"))
    {
      (void )fclose(fP);
    }
  }
  if(ok)
  {
    if((exp = WlzExpAssign(WlzExpParse(expStr, &nPar, par))) == NULL)
    {
      ok = 0;
      (void )fprintf(stderr,
                     "%s: failed to parse input string:\n%s\n",
		     *argv, expStr);
    }
  }
  if(ok)
  {
    if(verbose)
    {
      expStr2 = WlzExpStr(exp, NULL, &errNum);
    }
    if((errNum == WLZ_ERR_NONE) && (noEval == 0))
    {
      outObj = WlzAssignObject(WlzExpEval(inObj, INT_MAX, exp, &errNum), NULL);
    }
    if(errNum != WLZ_ERR_NONE)
    {
      ok = 0;   
      (void )WlzStringFromErrorNum(errNum, &errMsgStr);
      (void )fprintf(stderr,
                     "%s: failed to evaluate input string (%s):\n%s\n",
		     *argv, errMsgStr, expStr);
    }
  }
  if(ok && verbose)
  {
    (void )fprintf(stderr, "Given image processing expression was: %s\n",
                   expStr);
    (void )fprintf(stderr, "Expression string: %s\n", expStr2);
    (void )fprintf(stderr, "Number of parameters = %d\n", nPar);
    if(nPar > 0)
    {
      (void )fprintf(stderr, "Parameters:");
      for(i = 0; i < nPar; ++i)
      {
        (void )fprintf(stderr, " %d", par[i]);
      }
      (void )fprintf(stderr, "\n");
    }
  }
  AlcFree(expStr2);
  WlzExpFree(exp);
  if(ok)
  {
    fP = NULL;
    errNum = WLZ_ERR_WRITE_EOF;
    if(((fP = (strcmp(outFileStr, "-")?
              fopen(outFileStr, "w"): stdout)) == NULL) ||
       ((errNum = WlzWriteObj(fP, outObj)) != WLZ_ERR_NONE))
    {
      ok = 0;
      (void )fprintf(stderr,
                     "%s: failed to write output object to file %s\n",
		     *argv, outFileStr);
    }
    if(fP && strcmp(outFileStr, "-"))
    {
      (void )fclose(fP);
    }
  }
  (void )WlzFreeObj(inObj);
  (void )WlzFreeObj(outObj);
  if(usage)
  {
    (void )fprintf(stderr,
     	"Usage: %s [-h] [-o <out>] [-s <str>] [-v] [<in obj>]\n"
     	"Reads an object and evaluates an image processing expression\n"
	"using it.\n"
        "Options are:\n"
        "  -h  Shows this usage message.\n"
        "  -n  Don't evaluate the expression.\n"
        "  -o  Output object file.\n"
        "  -s  Image processing expression.\n"
        "  -v  Verbose output to stderr.\n",
        *argv);
    ok = 0;
  }
  return(!ok);
}
