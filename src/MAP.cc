#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _MAP_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         MAP.cc
* \author       Bill Hill
* \date         July 2012
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
* \brief	Provides the MAP command of the WlzIIPServer.
* \ingroup	WlzIIPServer
*/

#include "Log.h"
#include "Task.h"

using namespace std;

/*!
* \ingroup	WlzIIPServer
* \brief	Parses a map command and then stores the parsed command for
* 		later evaluation.
* 		See ImageMap::parse() for the syntax of the map command.
* \param	session			The session.
* \param	argument		String to parse for the map parameters.
*/
void MAP::run(Session* session, std::string argument)
{

  int		nChan;

  LOG_INFO("MAP handler reached");
  ImageMap	&map = session->viewParams->map;
  if(map.parse(argument.c_str()))
  {
    throw string("MAP : Failed to parse mapping.");
  }
  nChan = map.getNChan();
  LOG_INFO("MAP :: Map channels        : " << nChan);
  for(int i = 0; i < nChan; ++i)
  {
    const ImageMapChan *chan = map.getChan(i);
    LOG_INFO("MAP :: Mapping fn [" << i << "] : " <<
             ImageMap::mapTypeToString(chan->type));
    LOG_INFO("MAP :: Mapping il [" << i << "] : " << chan->il);
    LOG_INFO("MAP :: Mapping iu [" << i << "] : " << chan->iu);
    LOG_INFO("MAP :: Mapping ol [" << i << "] : " << chan->ol);
    LOG_INFO("MAP :: Mapping ou [" << i << "] : " << chan->ou);
    LOG_INFO("MAP :: Mapping p0 [" << i << "] : " << chan->p0);
    LOG_INFO("MAP :: Mapping p1 [" << i << "] : " << chan->p1);
  }
}
