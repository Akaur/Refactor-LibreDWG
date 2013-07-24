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
 * decode.h: decoding function prototypes
 * written by Felipe Castro
 * modified by Felipe Corrêa da Silva Sances
 * modified by Rodrigo Rodrigues da Silva
 */

#ifndef DECODE_H
#define DECODE_H

#ifndef _DWG_OBJECT_H_
#define _DWG_OBJECT_H_

#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "bits.h"
#include "dwg.h"
#include "print.h"

/*--------------------------------------------------------------------------------
 * Welcome to the dark side of the moon...
 * MACROS
 */

#define IS_DECODER

#define FIELD(name,type)\
  _obj->name = bit_read_##type(dat);\
  LOG_TRACE(#name ": " FORMAT_##type "\n", _obj->name)

#define FIELD_VALUE(name) _obj->name

#define ANYCODE -1
#define FIELD_HANDLE(name, handle_code)\
  if (handle_code>=0)\
    {\
      _obj->name = dwg_decode_handleref_with_code(dat, obj, dwg, handle_code);\
    }\
  else\
    {\
      _obj->name = dwg_decode_handleref(dat, obj, dwg);\
    }\
  LOG_TRACE(#name ": HANDLE(%d.%d.%lu) absolute:%lu\n",\
        _obj->name->handleref.code,\
        _obj->name->handleref.size,\
        _obj->name->handleref.value,\
        _obj->name->absolute_ref)

#define FIELD_B(name) FIELD(name, B);
#define FIELD_BB(name) FIELD(name, BB);
#define FIELD_BS(name) FIELD(name, BS);
#define FIELD_BL(name) FIELD(name, BL);
#define FIELD_BD(name) FIELD(name, BD);
#define FIELD_RC(name) FIELD(name, RC);
#define FIELD_RS(name) FIELD(name, RS);
#define FIELD_RD(name) FIELD(name, RD);
#define FIELD_RL(name) FIELD(name, RL);
#define FIELD_MC(name) FIELD(name, MC);
#define FIELD_MS(name) FIELD(name, MS);
#define FIELD_TV(name) FIELD(name, TV);
#define FIELD_T FIELD_TV /*TODO: implement version dependant string fields */
#define FIELD_BT(name) FIELD(name, BT);
#define FIELD_4BITS(name) _obj->name = bit_read_4BITS(dat);

#define FIELD_BE(name) bit_read_BE(dat, &_obj->name.x, &_obj->name.y, &_obj->name.z);
#define FIELD_DD(name, _default) FIELD_VALUE(name) = bit_read_DD(dat, _default);
#define FIELD_2DD(name, d1, d2) FIELD_DD(name.x, d1); FIELD_DD(name.y, d2);
#define FIELD_2RD(name) FIELD(name.x, RD); FIELD(name.y, RD);
#define FIELD_2BD(name) FIELD(name.x, BD); FIELD(name.y, BD);
#define FIELD_3RD(name) FIELD(name.x, RD); FIELD(name.y, RD); FIELD(name.z, RD);
#define FIELD_3BD(name) FIELD(name.x, BD); FIELD(name.y, BD); FIELD(name.z, BD);
#define FIELD_3DPOINT(name) FIELD_3BD(name)
#define FIELD_CMC(name)\
    bit_read_CMC(dat, &_obj->name);\
    LOG_TRACE(#name ": index %d\n", _obj->name.index)

//FIELD_VECTOR_N(name, type, size):
// reads data of the type indicated by 'type' 'size' times and stores
// it all in the vector called 'name'.
#define FIELD_VECTOR_N(name, type, size)\
  if (size>0)\
    {\
      _obj->name = (BITCODE_##type*) malloc(size * sizeof(BITCODE_##type));\
      for (vcount=0; vcount< size; vcount++)\
        {\
          _obj->name[vcount] = bit_read_##type(dat);\
          LOG_TRACE(#name "[%d]: " FORMAT_##type "\n", vcount, _obj->name[vcount])\
        }\
    }

#define FIELD_VECTOR(name, type, size) FIELD_VECTOR_N(name, type, _obj->size)

#define FIELD_2RD_VECTOR(name, size)\
  _obj->name = (BITCODE_2RD *) malloc(_obj->size * sizeof(BITCODE_2RD));\
  for (vcount=0; vcount< _obj->size; vcount++)\
    {\
      FIELD_2RD(name[vcount]);\
    }

#define FIELD_2DD_VECTOR(name, size)\
  _obj->name = (BITCODE_2RD *) malloc(_obj->size * sizeof(BITCODE_2RD));\
  FIELD_2RD(name[0]);\
  for (vcount = 1; vcount < _obj->size; vcount++)\
    {\
      FIELD_2DD(name[vcount], FIELD_VALUE(name[vcount - 1].x), FIELD_VALUE(name[vcount - 1].y));\
    }

#define FIELD_3DPOINT_VECTOR(name, size)\
  _obj->name = (BITCODE_3DPOINT *) malloc(_obj->size * sizeof(BITCODE_3DPOINT));\
  for (vcount=0; vcount< _obj->size; vcount++)\
    {\
      FIELD_3DPOINT(name[vcount]);\
    }

#define HANDLE_VECTOR_N(name, size, code)\
  FIELD_VALUE(name) = (BITCODE_H*) malloc(sizeof(BITCODE_H) * size);\
  for (vcount=0; vcount<size; vcount++)\
    {\
      FIELD_HANDLE(name[vcount], code);\
    }

#define HANDLE_VECTOR(name, sizefield, code) HANDLE_VECTOR_N(name, FIELD_VALUE(sizefield), code)

#define FIELD_XDATA(name, size)\
  _obj->name = dwg_decode_xdata(dat, _obj->size)

#define REACTORS(code)\
  FIELD_VALUE(reactors) = (BITCODE_H*) malloc(sizeof(BITCODE_H) * obj->tio.object->num_reactors);\
  for (vcount=0; vcount<obj->tio.object->num_reactors; vcount++)\
    {\
      FIELD_HANDLE(reactors[vcount], code);\
    }

#define ENT_REACTORS(code)\
  FIELD_VALUE(reactors) = (BITCODE_H*) malloc(sizeof(BITCODE_H) * obj->tio.entity->num_reactors);\
  for (vcount=0; vcount<obj->tio.entity->num_reactors; vcount++)\
    {\
      FIELD_HANDLE(reactors[vcount], code);\
    }

#define XDICOBJHANDLE(code)\
  SINCE(R_2004)\
    {\
      if (!obj->tio.object->xdic_missing_flag)\
        {\
          FIELD_HANDLE(xdicobjhandle, code);\
        }\
    }\
  PRIOR_VERSIONS\
    {\
      FIELD_HANDLE(xdicobjhandle, code);\
    }

#define ENT_XDICOBJHANDLE(code)\
  SINCE(R_2004)\
    {\
      if (!obj->tio.entity->xdic_missing_flag)\
        {\
          FIELD_HANDLE(xdicobjhandle, code);\
        }\
    }\
  PRIOR_VERSIONS\
    {\
      FIELD_HANDLE(xdicobjhandle, code);\
    }

#define REPEAT_N(times, name, type) \
  _obj->name = (type *) malloc(times * sizeof(type));\
  for (rcount=0; rcount<times; rcount++)

#define REPEAT(times, name, type) \
  _obj->name = (type *) malloc(_obj->times * sizeof(type));\
  for (rcount=0; rcount<_obj->times; rcount++)

#define REPEAT2(times, name, type) \
  _obj->name = (type *) malloc(_obj->times * sizeof(type));\
  for (rcount2=0; rcount2<_obj->times; rcount2++)

#define REPEAT3(times, name, type) \
  _obj->name = (type *) malloc(_obj->times * sizeof(type));\
  for (rcount3=0; rcount3<_obj->times; rcount3++)

//TODO unify REPEAT macros!

#define COMMON_ENTITY_HANDLE_DATA \
  dwg_decode_common_entity_handle_data(dat, obj)

#define DWG_ENTITY(token) \
void \
 dwg_decode_##token (Bit_Chain * dat, Dwg_Object * obj)\
{\
  int vcount, rcount, rcount2, rcount3;\
  Dwg_Entity_##token *ent, *_obj;\
  Dwg_Data* dwg = obj->parent;\
  LOG_INFO("Entity " #token ":\n")\
  dwg->num_entities++;\
  obj->supertype = DWG_SUPERTYPE_ENTITY;\
  obj->tio.entity = (Dwg_Object_Entity*)malloc (sizeof (Dwg_Object_Entity));	\
  obj->tio.entity->tio.token = (Dwg_Entity_##token *)calloc (sizeof (Dwg_Entity_##token), 1); \
  ent = obj->tio.entity->tio.token;\
  _obj=ent;\
  obj->tio.entity->object = obj;\
  if (dwg_decode_entity (dat, obj->tio.entity)) return;\
  LOG_INFO("Entity handle: %d.%d.%lu\n",\
    obj->handle.code,\
    obj->handle.size,\
    obj->handle.value)

#define DWG_ENTITY_END }

#define DWG_OBJECT(token) void  dwg_decode_ ## token (Bit_Chain * dat, Dwg_Object * obj) {\
  int vcount, rcount, rcount2, rcount3;\
  Dwg_Object_##token *_obj;\
  Dwg_Data* dwg = obj->parent;\
  LOG_INFO("Object " #token ":\n")\
  obj->supertype = DWG_SUPERTYPE_OBJECT;\
  obj->tio.object = (Dwg_Object_Object*)malloc (sizeof (Dwg_Object_Object));	\
  obj->tio.object->tio.token = (Dwg_Object_##token * ) calloc (sizeof (Dwg_Object_##token), 1); \
  obj->tio.object->object = obj;\
  if (dwg_decode_object (dat, obj->tio.object)) return;\
  _obj = obj->tio.object->tio.token;\
  LOG_INFO("Object handle: %d.%d.%lu\n",\
    obj->handle.code,\
    obj->handle.size,\
    obj->handle.value)

#define DWG_OBJECT_END }

int
dwg_decode_data(Bit_Chain * bit_chain, Dwg_Data * dwg_data);

int
dwg_decode_entity(Bit_Chain * dat, Dwg_Object_Entity * ent);

int
dwg_decode_object(Bit_Chain * dat, Dwg_Object_Object * ord);

Dwg_Object_Ref *
dwg_decode_handleref(Bit_Chain * dat, Dwg_Object * obj, Dwg_Data* dwg);

Dwg_Object_Ref *
dwg_decode_handleref_with_code(Bit_Chain * dat, Dwg_Object * obj,
                               Dwg_Data* dwg, unsigned int code);

void
dwg_decode_common_entity_handle_data(Bit_Chain * dat, Dwg_Object * obj);

Dwg_Resbuf*
dwg_decode_xdata(Bit_Chain * dat, int size);

#endif

#undef IS_DECODER

#endif
