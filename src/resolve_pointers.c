/*****************************************************************************/
/*  LibreDWG - free implementation of the DWG file format                    */
/*                                                                           */
/*  Copyright (C) 2009 Free Software Foundation, Inc.                        */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 3 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

/*
 * decode.c: decoding functions
 * written by Felipe Castro
 * modified by Felipe Corrêa da Silva Sances
 * modified by Rodrigo Rodrigues da Silva
 * modified by Till Heuschmann
 */


#include "resolve_pointers.h"
#include "logging.h"
#include "decode.h"

void
resolve_objectref_vector(Dwg_Data * dwg)
{
  long unsigned int i;
  Dwg_Object * obj;
  for (i = 0; i < dwg->num_object_refs; i++)
    {
      LOG_TRACE("\n==========\n")
      LOG_TRACE("-objref: HANDLE(%d.%d.%lu) Absolute:%lu\n",
          dwg->object_ref[i]->handleref.code,
          dwg->object_ref[i]->handleref.size,
          dwg->object_ref[i]->handleref.value,
          dwg->object_ref[i]->absolute_ref)

      //look for object
      obj = dwg_resolve_handle(dwg, dwg->object_ref[i]->absolute_ref);

      if(obj)
        {
          LOG_TRACE("-found:  HANDLE(%d.%d.%lu)\n",
              obj->handle.code,
              obj->handle.size,
              obj->handle.value)
        }

      //assign found pointer to objectref vector
      dwg->object_ref[i]->obj = obj;


      if (DWG_LOGLEVEL >= DWG_LOGLEVEL_INSANE)
        {
          if (obj)
            dwg_print_object(obj);
          else
            LOG_ERROR("Null object pointer: object_ref[%lu]\n", i)
        }
    }
}

/**
 * Find a pointer to an object given it's id (handle)
 */
Dwg_Object *
dwg_resolve_handle(Dwg_Data* dwg, long unsigned int absref)
{
  //FIXME find a faster algorithm
  long unsigned int i;
  for (i = 0; i < dwg->num_objects; i++)
    {
      if (dwg->object[i].handle.value == absref)
        {
          return &dwg->object[i];
        }
    }
  LOG_ERROR("Object not found: %lu\n", absref)
  return 0;
}

