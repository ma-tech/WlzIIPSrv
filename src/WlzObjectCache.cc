#if defined(__GNUC__)
#ident "University of Edinburgh $Id$"
#else
static char _WlzObjectCache_cc[] = "University of Edinburgh $Id$";
#endif
/*!
* \file         WlzObjectCache.cc
* \author       Bill Hill
* \date         December 2011
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
* \brief	The caching mechanism for Woolz Objects.
* \ingroup	WlzIIPServer
*/

#include "Log.h"
#include "WlzObjectCache.h"

/*!
* \ingroup  WlzIIPServer
* \brief    Constructor for WlzObjectCache.
*/
WlzObjectCache::
WlzObjectCache()
{
  unsigned int 	maxItem = 0;
  size_t	 maxSz;
  
  enabled = 1;
  maxItem = Environment::getMaxWlzObjCacheCount();
  maxSz = MBytesToBytes(Environment::getMaxWlzObjCacheSize());
  objCache = AlcLRUCacheNew(maxItem, maxSz,
			    (AlcLRUCKeyFn )this->WlzObjCacheKeyFn,
			    (AlcLRUCCmpFn )this->WlzObjCacheCmpFn,
			    (AlcLRUCUnlinkFn )this->WlzObjCacheUnlinkFn,
			    NULL);
  LOG_INFO("WlzObjectCache initialised with maxItem=" << maxItem <<
            " and maxSz = " << maxSz << " success=" <<
	     (objCache != NULL)? 1: 0);
}

/*!
* \ingroup  WlzIIPServer
* \brief    Destructor for WlzObjectCache.
*/
WlzObjectCache::
~WlzObjectCache()
{
  LOG_NOTICE("WlzObjectCache released.\n");
  AlcLRUCacheFree(objCache, 1);
}

/*!
* \return	Object size in megabytes.
* \ingroup	WlzIIPServer
* \brief	Computes the approximate object size in bytes.
* \param	obj			The object.
*/
size_t		WlzObjectCache::
		ComputeObjectSize(WlzObject *obj)
{
  size_t	sz = 0;
  WlzErrorNum	errNum = WLZ_ERR_NONE;

  if(obj && obj->domain.core)
  {
    switch(obj->type)
    {
      case WLZ_EMPTY_OBJ:
	break;
      case WLZ_2D_DOMAINOBJ: /* FALLTHROUGH */
      case WLZ_3D_DOMAINOBJ:
	sz = 4 * WlzIntervalCountObj(obj, &errNum);
	if((errNum == WLZ_ERR_NONE) &&
	   obj->values.core && !WlzGreyTableIsTiled(obj->values.core->type))
	{
	  size_t sz1;

	  WlzGreyType gType;
	  sz1 = WlzVolume(obj, &errNum);
	  gType = WlzGreyTypeFromObj(obj, &errNum);
	  if(errNum == WLZ_ERR_NONE)
	  {
	    sz1 *= WlzGreySize(gType);
	  }
	  sz += sz1;
	}
	break;
      case WLZ_CMESH_2D:  /* FALLTHROUGH */
      case WLZ_CMESH_2D5: /* FALLTHROUGH */
      case WLZ_CMESH_3D:
	{
	  size_t	mN,
		    	mE;

	  switch(obj->type)
	  {
	    case WLZ_CMESH_2D:
	      sz = sizeof(WlzCMesh2D);
	      mN = obj->domain.cm2->res.nod.maxEnt;
	      mE = obj->domain.cm2->res.elm.maxEnt;
	      sz += (mN * sizeof(WlzCMeshNod2D)) +
		    (mE * sizeof(WlzCMeshElm2D));
	      break;
	    case WLZ_CMESH_2D5:
	      sz = sizeof(WlzCMesh2D5);
	      mN = obj->domain.cm2d5->res.nod.maxEnt;
	      mE = obj->domain.cm2d5->res.elm.maxEnt;
	      sz += (mN * sizeof(WlzCMeshNod2D5)) +
		    (mE * sizeof(WlzCMeshElm2D5));
	      break;
	    case WLZ_CMESH_3D:
	      sz = sizeof(WlzCMesh3D);
	      mN = obj->domain.cm3->res.nod.maxEnt;
	      mE = obj->domain.cm3->res.elm.maxEnt;
	      sz += (mN * sizeof(WlzCMeshNod3D)) +
		    (mE * sizeof(WlzCMeshElm3D));
	      break;
	    default:
	      break;
	  }
	  if(obj->values.core)
	  {
	    int		i;
	    size_t 	t;
	    WlzIndexedValues *x;

	    x = obj->values.x;
	    sz += sizeof(WlzIndexedValues);
	    t = 1;
	    for(i = 0; i < x->rank; ++i)
	    {
	      t *= x->dim[i];
	    }
	    switch(x->attach)
	    {
	      case WLZ_VALUE_ATTACH_NOD:
		sz += mN * t * WlzGreySize(x->vType);
		break;
	      case WLZ_VALUE_ATTACH_ELM:
		sz += mE * t * WlzGreySize(x->vType);
		break;
	      default:
		errNum = WLZ_ERR_VALUES_DATA;
		break;
	    }
	  }
	}
	break;
      case WLZ_AFFINE_TRANS:
	sz = sizeof(WlzAffineTransform) + (4 * 4 * sizeof(double));
	break;
      case WLZ_LUT:
	sz = sizeof(WlzLUTDomain);
	if(obj->values.core)
	{
	  WlzGreyType gType;
	  gType = obj->values.lut->vType;
	  if(errNum == WLZ_ERR_NONE)
	  {
	    sz += sizeof(WlzLUTValues) +
		  WlzGreySize(gType) * obj->values.lut->maxVal;
	  }
	}
	break;
      case WLZ_COMPOUND_ARR_1: /* FALLTHROUGH */
      case WLZ_COMPOUND_ARR_2:
	{
	  int	i;
	  WlzCompoundArray	*c;

	  c = (WlzCompoundArray *)obj;
	  for(i = 0; i < c->n; ++i)
	  {
	    sz += ComputeObjectSize(c->o[i]);
	    if(errNum != WLZ_ERR_NONE)
	    {
	      break;
	    }
	  }
	}
	break;
      case WLZ_3D_VIEW_STRUCT:
	sz = sizeof(WlzThreeDViewStruct) +
	     sizeof(WlzAffineTransform) + (4 * 4 * sizeof(double));;
	break;
      default:
	errNum = WLZ_ERR_OBJECT_TYPE;
	break;
    }
  }
  return(sz);
}

/*!
* \ingroup  	WlzIIPServer
* \brief	Computes a cache key from a cache entry identification string.
* \param	cache			The cache (unused).
* \param	e			Cache entry. Only the identification
* 					string field is used.
*/
unsigned int	WlzObjectCache::
		WlzObjCacheKeyFn(AlcLRUCache *cache, const void *e)
{
  unsigned int	key;
  WlzObjCacheEntry *ent;

  ent = (WlzObjCacheEntry *)e;
  key = AlcStrSFHash(ent->str);
  return(key);
}

/*!
* \ingroup  	WlzIIPServer
* \brief	Compares two cache entries using their identification string.
* 		Only the identification string fields are used.
* \param	e0			First cache entry.
* \param	e1			Seconds cache entry.
*/
int		WlzObjectCache::
		WlzObjCacheCmpFn(const void *e0, const void *e1)
{
  int		cmp;
  WlzObjCacheEntry  *ent0,
		    *ent1;

  ent0 = (WlzObjCacheEntry *)e0;
  ent1 = (WlzObjCacheEntry *)e1;
  cmp = strcmp(ent0->str, ent1->str);
  return(cmp);
}

/*!
* \ingroup	WlzIIPServer
* \brief	This function is called when a cache entry is about to be
* 		removed. This function frees the entry object and then the
* 		entry itself.
* \param	cache			The cache (unused).
* \param	e			Cache entry.
*/
void		WlzObjectCache::
		WlzObjCacheUnlinkFn(AlcLRUCache *cache, const void *e)
{
  WlzObjCacheEntry *ent;

  if((ent = (WlzObjCacheEntry *)e) != NULL)
  {
    (void )WlzFreeObj(ent->obj);
    AlcFree(ent);
  }
}

/*!
* \ingroup	WlzIIPServer
* \brief    	Inserts a Woolz 3D view data structure into the
* 		cache as an object.
* \warning	Objects may not be cached if too big to fit.
* \param    	obj       		Woolz 3D view data structure to be
* 					inserted
* \param    	str	  		String used to identify the object.
*/
void 		WlzObjectCache::
		insert(WlzThreeDViewStruct *vs, const std::string  str)
	    	throw(std::string)
{
  if(enabled)
  {
    WlzObject	*obj;
    WlzDomain	dom;
    WlzValues	val;

    dom.vs3d = vs;
    val.core = NULL;
    obj = WlzMakeMain(WLZ_3D_VIEW_STRUCT, dom, val, NULL, NULL, NULL);
    if(obj == NULL)
    {
      throw string(
	"WlzObjectCache::insert - failed to make object from view struct.");
    }
    this->insert(obj, str);
  }
}

/*!
* \ingroup  	WlzIIPServer
* \brief	Inserts a Woolz object.
* \warning	Objects may not be cached if too big to fit.
* \param    	obj       		WlzObj to be be inserted
* \param    	str	  		String used to identify the object.
*/
void 		WlzObjectCache::
		insert(WlzObject *obj, const std::string  str)
		throw(std::string)
{
  LOG_INFO("WlzObjectCache::insert " << str);
  if(enabled)
  {
    WlzObjCacheEntry *ent = NULL;

    if(((ent = (WlzObjCacheEntry *)
	       AlcCalloc(1, sizeof(WlzObjCacheEntry))) != NULL) &&
       ((ent->str = AlcStrDup(str.c_str())) != NULL))
    {
      int	newFlg = 0;
      unsigned int key;
      AlcLRUCItem *item = NULL;

      ent->obj = NULL;
      key = this->WlzObjCacheKeyFn(objCache, ent);
      item = AlcLRUCItemFind(objCache, key, (void *)ent);
      LOG_INFO("WlzObjectCache::insert item in cache=" <<
	       (item != NULL)? 1: 0);
      if(item == NULL)
      {
        size_t	sz;

	ent->obj = WlzAssignObject(obj, NULL);
	sz = ComputeObjectSize(obj);
	item = AlcLRUCEntryAddWithKey(objCache, sz, ent, key, &newFlg);
	LOG_INFO("WlzObjectCache::insert sz=" << sz);
      }
      LOG_INFO("WlzObjectCache::insert item added to cache=" <<
	       (newFlg != 0)? 1: 0);
      if(newFlg == 0)
      {
	(void )WlzFreeObj(ent->obj);
	AlcFree(ent->str);
	AlcFree(ent);
      }
    }
    else
    {
      AlcFree(ent);
      throw string("WlzObjectCache::insert - memory allocation failure.");
    }
  }
}

/*!
* \return	Pointer to the requested Woolz object or NULL if not found
* 		in the cache.
* \ingroup  	WlzIIPServer
* \brief    	Gets a Woolz object from the cache .
* \param    	str     		String identifying the required
* 					object.
*/
WlzObject 	*WlzObjectCache::
		get(std::string str)
{
  WlzObject	*obj = NULL;

  if(enabled)
  {
    unsigned int	key;
    WlzObjCacheEntry ent;
    AlcLRUCItem *item;

    ent.str = (char *)(str.c_str());
    key = this->WlzObjCacheKeyFn(objCache, &ent);
    item = AlcLRUCItemFind(objCache, key, &ent);
    if(item)
    {
      obj = ((WlzObjCacheEntry *)(item->entry))->obj;
    }
#ifdef WLZ_IIP_LOG
    if(obj)
    {
      LOG_INFO("WlzObjectCache::get str=" << str <<
               " obj=1 obj->linkcount=" << obj->linkcount);
    }
    else
    {
      LOG_INFO("WlzObjectCache::get str=" << str << " obj=0");
    }
#endif
  }
  return(obj);
}

/*!
* \return	Pointer to the requested Woolz 3D view structure or NULL if
*           	not found in the cache.
* \ingroup	WlzIIPServer
* \brief    	Gets a Woolz 3D view structure from the cache .
* \param    	str     		String identifying the required
* 					3D view structure object.
*/
WlzThreeDViewStruct *WlzObjectCache::
		     getVS(std::string str)
{
  WlzThreeDViewStruct *vs = NULL;

  if(enabled)
  {
    unsigned int	key;
    WlzObjCacheEntry ent;
    AlcLRUCItem *item;

    ent.str = (char *)(str.c_str());
    key = this->WlzObjCacheKeyFn(objCache, &ent);
    item = AlcLRUCItemFind(objCache, key, &ent);
    if(item)
    {
      WlzObject *obj;

      obj = ((WlzObjCacheEntry *)(item->entry))->obj;
      if(obj && (obj->type = WLZ_3D_VIEW_STRUCT))
      {
	vs = obj->domain.vs3d;
      }
    }
  }
  return(vs);
}

/*!
* \return   	The number of objects cached.
* \ingroup	WlzIIPServer
* \brief    	Returns the number of objects cached.
*/
unsigned int 	WlzObjectCache::
		getNumElements()
{
  unsigned int n = 0;

  if(objCache)
  {
    n = objCache->numItem;
  }
  return(n);
}

/*!
* \return	The number of objects cached
* \ingroup	WlzIIPServer
* \brief    	Returns the number of MB stored in the object cache.
*/
float 		WlzObjectCache::
		getMemorySize()
{
  return((float )BytesToMBytes(objCache->curSz));
}

/*!
* \ingroup	WlzIIPServer
* \brief    	Sets the maximum cache size. If size is less the currently
* 	        stored objects, the objects are freed.
* \param    	max 			Maximum cache size in bytes.
*/
void 		WlzObjectCache::
		setMaxSize(size_t max)
{
  if(objCache)
  {
    AlcLRUCacheMaxSz(objCache, BytesToMBytes(max));
  }
};
