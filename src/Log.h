#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _Log_h[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         Log.h
* \author       Bill Hill
* \date         February 2012
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
* \brief	Simple wrappers for using the log4cpp logging system
* 		in the Woolz IIP server. The log priority levels used
* 		are:
*               <ul>
                <li>FATAL
		  Errors that cause the server to fail to start or exit.
                <li>ERROR
		  Operation failure but server still OK.
                <li>WARN
		  Inappropriate input but server still OK.
                <li>NOTICE
		  Normal logging with little impact on performance.
                <li>INFO
		  Verbose logging with some impact on performance.
                <li>DEBUG
		  Very verbose and logging with a big impact on performance.
               </ul>
*
* \ingroup	WlzIIPSrv
*/
#ifndef _LOG_H
#define _LOG_H

#ifdef HAVE_LOG4CPP
#define WLZ_IIP_LOG  
#endif

#ifdef WLZ_IIP_LOG
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/Priority.hh>

#ifdef _MAIN_CC
log4cpp::Category *logCat = NULL;
#else
extern log4cpp::Category *logCat;
#endif

#define LOG_CONCAT(M)       std::stringstream s;s<<M;
#define LOG_COND_EMERG(X)   if((logCat!=NULL)&&(logCat->isEmergEnabled())){X;}
#define LOG_COND_FATAL(X)   if((logCat!=NULL)&&(logCat->isFatalEnabled())){X;}
#define LOG_COND_ALERT(X)   if((logCat!=NULL)&&(logCat->isAlertEnabled())){X;}
#define LOG_COND_CRIT(X)    if((logCat!=NULL)&&(logCat->isCritEnabled())){X;}
#define LOG_COND_ERROR(X)   if((logCat!=NULL)&&(logCat->isErrorEnabled())){X;}
#define LOG_COND_WARN(X)    if((logCat!=NULL)&&(logCat->isWarnEnabled())){X;}
#define LOG_COND_NOTICE(X)  if((logCat!=NULL)&&(logCat->isNoticeEnabled())){X;}
#define LOG_COND_INFO(X)    if((logCat!=NULL)&&(logCat->isInfoEnabled())){X;}
#define LOG_COND_DEBUG(X)   if((logCat!=NULL)&&(logCat->isDebugEnabled())){X;}
#define LOG_EMERG(M)        \
  LOG_COND_EMERG(std::stringstream logS;logS<<M;logCat->emerg(logS.str()))
#define LOG_FATAL(M)        \
  LOG_COND_FATAL(std::stringstream logS;logS<<M;logCat->fatal(logS.str())) 
#define LOG_ALERT(M)        \
  LOG_COND_ALERT(std::stringstream logS;logS<<M;logCat->alert(logS.str())) 
#define LOG_CRIT(M)         \
  LOG_COND_CRIT(std::stringstream logS;logS<<M;logCat->crit(logS.str())) 
#define LOG_ERROR(M)        \
  LOG_COND_ERROR(std::stringstream logS;logS<<M;logCat->error(logS.str())) 
#define LOG_WARN(M)         \
  LOG_COND_WARN(std::stringstream logS;logS<<M;logCat->warn(logS.str())) 
#define LOG_NOTICE(M)       \
  LOG_COND_NOTICE(std::stringstream logS;logS<<M;logCat->notice(logS.str())) 
#define LOG_INFO(M)         \
  LOG_COND_INFO(std::stringstream logS;logS<<M;logCat->info(logS.str())) 
#define LOG_DEBUG(M)        \
  LOG_COND_DEBUG(std::stringstream logS;logS<<M;logCat->debug(logS.str())) 
#else
#define LOG_COND_EMERG(X)
#define LOG_COND_FATAL(X)
#define LOG_COND_ALERT(X)
#define LOG_COND_CRIT(X)
#define LOG_COND_ERROR(X)
#define LOG_COND_WARN(X)
#define LOG_COND_NOTICE(X)
#define LOG_COND_INFO(X)
#define LOG_COND_DEBUG(X)
#define LOG_EMERG(M)
#define LOG_FATAL(M)
#define LOG_ALERT(M)
#define LOG_CRIT(M)
#define LOG_ERROR(M)
#define LOG_WARN(M)
#define LOG_NOTICE(M)
#define LOG_INFO(M)
#define LOG_DEBUG(M)
#endif

#endif
