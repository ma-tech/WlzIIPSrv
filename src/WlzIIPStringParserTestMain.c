#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzIIPStringParserTestMain_c[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzIIPStringParserTestMain.c
* \author       Bill Hill
* \date         June 2016
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               MRC Institute of Genetics and Molecular Medicine,
*               University of Edinburgh,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C), [2016],
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
* \brief	This is a test program for various string parsing
* 		functions used in the Woolz IIP server.
* \ingroup	WlzIIPServer
*/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <Wlz.h>
#include "WlzIIPStringParser.h"

int 		main(int argc, char *argv[])
{
  int		option,
  		ok = 1,
		nPos = 0,
		nIdx = 0,
  		usage = 0,
		verbose = 0;
  char		*str = NULL;
  int		*idx = NULL;
  WlzDVertex3	*pos = NULL;
  static char	optList[] = "hvs:";
  char 		strDef[]="0,(1.23e03,-23.4,+3),(-4,5.0e-02,6.07),+7,8";


  str = strDef;
  while((usage == 0) && ((option = getopt(argc, argv, optList)) != EOF))
  {
    switch(option)
    {
      case 's':
        str = optarg;
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
  if((usage == 0) && (optind != argc))
  {
    usage = 1;
  }
  ok = usage == 0;
  if(ok && verbose)
  {
    (void )printf("str = %s\n", str);
  }
  if(ok)
  {
    int		n;

    n = strlen(str) + 3;
    if(((pos = AlcMalloc(sizeof(WlzDVertex3) * (n / 3))) == NULL) ||
       ((idx = AlcMalloc(sizeof(int) * n)) == NULL))
    {
      ok = 0;
      (void )fprintf(stderr,
                     "%s: failed to allocate buffers.\n",
		     *argv);
    }
  }
  if(ok)
  {
    int 	err;

    err = WlzIIPStrParseIdxAndPos(&nIdx, idx, &nPos, pos, str);
    if(err)
    {
      ok = 0;
      (void )fprintf(stderr,
                     "%s: failed to parse string (possible error at %d)\n",
		     *argv, err - 1);
    }
  }
  if(ok && verbose)
  {
    int		i;

    (void )printf("nIdx = %d\n", nIdx);
    for(i = 0; i < nIdx; ++i)
    {
      (void )printf("  idx[% 8d] = %d\n", i, idx[i]);
    }
    (void )printf("nPos = %d\n", nPos);
    for(i = 0; i < nPos; ++i)
    {
      (void )printf("  pos[% 8d] = %g,%g,%g\n",
                    i, pos[i].vtX, pos[i].vtY, pos[i].vtZ);
    }
  }
  AlcFree(pos);
  AlcFree(idx);
  if(usage)
  {
    (void )fprintf(stderr,
     	"Usage: %s [-h] [-s <str>] [-v]\n"
     	"Tests string parsing functions used with the Woolz IIP server.\n"
        "Options are:\n"
        "  -h  Shows this usage message.\n"
        "  -s  String to parse.\n"
        "  -v  Verbose output to stderr.\n",
        *argv);
    ok = 0;
  }
  return(!ok);
}
