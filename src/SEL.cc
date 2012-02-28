#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _SEL_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         SEL.cc
* \author       Zsolt Husz, Bill Hill
* \date         February 2010
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
* \brief	Provides the SEL command of the WlzIIPServer.
* \ingroup	WlzIIPServer
*/

#include "Log.h"
#include "Task.h"

using namespace std;

/*!
* \ingroup	WlzIIPServer
* \brief	Parses a selection command and then stores the parsed
* 		command for later evaluation.
*
* 		The arguments of the selection command are 1, 2, 4 or 5 comma
* 		separated parameters:
*	  	<ul>
*	  	  <li> \verbatim expression \endverbatim
*	  	  <li> \verbatim expression,alpha \endverbatim
*	  	  <li> \verbatim expression,red,green,blue \endverbatim
*	          <li> \verbatim expression,red,green,blue,alpha \endverbatim
*  	  	</ul>
*		With the expression being a Woolz morphological expression.
* \param	session			The session.
* \param	argument		String to parse for the selection
* 					arguments.
*/
void SEL::run(Session* session, std::string argument)
{

  unsigned int	nPar;
  unsigned int	par[4];
  WlzExp	*exp = NULL;
  CompoundSelector *selector = new CompoundSelector;

  LOG_INFO("SEL handler reached");
  if((exp = WlzExpParse(argument.c_str(), &nPar, par)) == NULL)
  {
    throw string("SEL : Failed to parse expression.");
  }
  switch(nPar)
  {
    case 0:
      selector->r = 255;
      selector->g = 255;
      selector->b = 255;
      selector->a = 255;
      break;
    case 1:
      selector->r = 255;
      selector->g = 255;
      selector->b = 255;
      selector->a = par[0];
      break;
    case 3:
      selector->r = par[0];
      selector->g = par[1];
      selector->b = par[2];
      selector->a = 255;
      break;
    case 4:
      selector->r = par[0];
      selector->g = par[1];
      selector->b = par[2];
      selector->a = par[3];
      break;
    default:
      throw string("SEL : Invalid number of parameters.");
      break;
  }
  selector->expression = WlzExpAssign(exp);
  // Insert at the end of the list
  if (!session->viewParams->selector)
  { //first element
    selector->next = session->viewParams->selector;
    session->viewParams->selector = selector;
    if (!session->viewParams->lastsel)
    {
      session->viewParams->lastsel= selector;
    }
  }
  else
  {
    session->viewParams->lastsel->next = selector;
    session->viewParams->lastsel = selector;
  }
  LOG_INFO("SEL :: Selection argument  : " << argument);
  LOG_INFO("SEL :: Selection for r     : " << (int)selector->r);
  LOG_INFO("SEL :: Selection for g     : " << (int)selector->g);
  LOG_INFO("SEL :: Selection for b     : " << (int)selector->b);
  LOG_INFO("SEL :: Selection for alpha : " << (int)selector->a);
}
