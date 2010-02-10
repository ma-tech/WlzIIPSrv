#if defined(__GNUC__)
#ident "MRC HGU $Id$"
#else
#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma ident "MRC HGU $Id$"
#else
static char _SEL_cc[] = "MRC HGU $Id$";
#endif
#endif
/*!
* \file         SEL.cc
* \author       Zsolt Husz
* \date         February 2010
* \version      $Id$
* \par
* Address:
*               MRC Human Genetics Unit,
*               Western General Hospital,
*               Edinburgh, EH4 2XU, UK.
* \par
* Copyright (C) 2010 Medical research Council, UK.
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
* \brief	Provides the SEL command of the WlzIIPServer.
* \ingroup	WlzIIPServer
*/

#include "Task.h"

using namespace std;

void SEL::run( Session* session, std::string argument ){

  /* The argument should consist of 1,2, 4 or 5 comma separated values:
     index
     index, alpha
     index, r, g, b
     index, r, g, b, alpha
  */

  if( session->loglevel >= 3 ) (*session->logfile) << "SEL handler reached" << endl;

  // Parse the argument list
  int count = 0;
  int values[5];
  int delimitter = argument.find( "," );
  int pdelimitter = 0;
  while (delimitter >=0 && count < 5) {
     values[count] = atoi( argument.substr( pdelimitter, (delimitter-pdelimitter)).c_str() );
     pdelimitter = delimitter + 1;
     delimitter = argument.find( ",", delimitter + 1);
     count++;
  }
  if (count > 4) {
      throw string( "SEL : Too many parameters.");
  }
  values[count] = atoi( argument.substr( pdelimitter, argument.length()-pdelimitter + 1).c_str() );

  CompoundSelector *selector = new CompoundSelector;
  int index = values[0];
  selector->index = index;
  switch (count) {
      case 0:
          selector->r=255;
          selector->g=255;
          selector->b=255;
          selector->a=255;
          break;
      case 1:
          selector->r=255;
          selector->g=255;
          selector->b=255;
          selector->a=values[1];
          break;
      case 3:
          selector->r=values[1];
          selector->g=values[2];
          selector->b=values[3];
          selector->a=255;
          break;
      case 4:
          selector->r=values[1];
          selector->g=values[2];
          selector->b=values[3];
          selector->a=values[4];
          break;
      default:
          throw string( "SEL : Incorrect SEL syntax. Number of acccepted parameters are 1, 2, 4 or 5.");
   }

   //insert to the end of the list
   if (!session->viewParams->selector) { //first element
       selector->next = session->viewParams->selector;
       session->viewParams->selector = selector;
       if (!session->viewParams->lastsel)
           session->viewParams->lastsel= selector;
   } else {
       session->viewParams->lastsel->next =  selector;
       session->viewParams->lastsel =  selector;
   }

  if ( session->loglevel >= 3 ) {
    int i;
    *(session->logfile) << "SEL :: Selection for index  : " << (int)selector->index << endl;
    *(session->logfile) << "SEL :: Selection for r      : " << (int)selector->r << endl;
    *(session->logfile) << "SEL :: Selection for g      : " << (int)selector->g << endl;
    *(session->logfile) << "SEL :: Selection for b      : " << (int)selector->b << endl;
    *(session->logfile) << "SEL :: Selection for alpha  : " << (int)selector->a << endl;
  }
}
