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

#include "decode.h"
#include "logging.h"
#include "decode_r13_r15.h"
#include "decode_r2004.h"

// extern unsigned int
// bit_ckr8(unsigned int dx, unsigned char *adr, long n);

/* The logging level for the read (decode) path.  */
static unsigned int loglevel;

#ifdef USE_TRACING
/* This flag means we have checked the environment variable
   LIBREDWG_TRACE and set `loglevel' appropriately.  */
static bool env_var_checked_p;

#define DWG_LOGLEVEL loglevel
#endif  /* USE_TRACING */

#define REFS_PER_REALLOC 100

int read_r2007_meta_data(Bit_Chain *dat, Dwg_Data *dwg);

/*--------------------------------------------------------------------------------
 * Public function definitions
 */
int
dwg_decode_data(Bit_Chain * dat, Dwg_Data * dwg)
{
  char version[7];
  dwg->num_object_refs = 0;
  dwg->num_layers = 0;
  dwg->num_entities = 0;
  dwg->num_objects = 0;
  dwg->num_classes = 0;

#ifdef USE_TRACING
  /* Before starting, set the logging level, but only do so once.  */
  if (! env_var_checked_p)
    {
      char *probe = getenv ("LIBREDWG_TRACE");

      if (probe)
        loglevel = atoi (probe);
      env_var_checked_p = true;
    }
#endif  /* USE_TRACING */

  /* Version */
  dat->byte = 0;
  dat->bit = 0;
  strncpy(version, (const char *)dat->chain, 6);
  version[6] = '\0';

  dwg->header.version = 0;
  if (!strcmp(version, version_codes[R_13]))
    dwg->header.version = R_13;
  if (!strcmp(version, version_codes[R_14]))
    dwg->header.version = R_14;
  if (!strcmp(version, version_codes[R_2000]))
    dwg->header.version = R_2000;
  if (!strcmp(version, version_codes[R_2004]))
    dwg->header.version = R_2004;
  if (!strcmp(version, version_codes[R_2007]))
    dwg->header.version = R_2007;
  if (dwg->header.version == 0)
    {
      LOG_ERROR("Invalid or unimplemented version code! "
        "This file's version code is: %s\n", version)
      return -1;
    }
  dat->version = (Dwg_Version_Type)dwg->header.version;
  LOG_INFO("This file's version code is: %s\n", version)

  PRE(R_2000)
    {
      LOG_INFO(
          "WARNING: This version of LibreDWG is only capable of safely decoding version R2000 (code: AC1015) dwg-files.\n"
            "This file's version code is: %s Support for this version is still experimental."
            "It might crash or give you invalid output.\n", version)
      return decode_R13_R15(dat, dwg);
    }

  VERSION(R_2000)
    {
      return decode_R13_R15(dat, dwg);
    }

  VERSION(R_2004)
    {
      LOG_INFO(
          "WARNING: This version of LibreDWG is only capable of properly decoding version R2000 (code: AC1015) dwg-files.\n"
            "This file's version code is: %s\n This version is not yet actively developed."
            "It will probably crash and/or give you invalid output.\n", version)
      return decode_R2004(dat, dwg);
    }

  VERSION(R_2007)
    {
      LOG_INFO(
          "WARNING: This version of LibreDWG is only capable of properly decoding version R2000 (code: AC1015) dwg-files.\n"
            "This file's version code is: %s\n This version is not yet actively developed."
            "It will probably crash and/or give you invalid output.\n", version)
      return decode_R2007(dat, dwg);
    }

  //This line should not be reached!
  LOG_ERROR(
      "ERROR: LibreDWG does not support this version: %s.\n",
      version)
  return -1;
}

int
decode_R2007(Bit_Chain* dat, Dwg_Data * dwg)
{
  int i;
  unsigned long int preview_address, security_type, unknown_long,
      dwg_property_address, vba_proj_address, app_info_address;
  unsigned char sig, DwgVer, MaintReleaseVer;

  /* 5 bytes of 0x00 */
  dat->byte = 0x06;
  LOG_TRACE("5 bytes of 0x00: ")
  for (i = 0; i < 5; i++)
    {
      sig = bit_read_RC(dat);
      if (loglevel)
        LOG_TRACE("0x%02X ", sig)
    }
  LOG_TRACE("\n")

  /* Unknown */
  dat->byte = 0x0B;
  sig = bit_read_RC(dat);
  LOG_TRACE("Unknown: 0x%02X\n", sig)

  /* Byte 0x00, 0x01, or 0x03 */
  dat->byte = 0x0C;
  sig = bit_read_RC(dat);
  LOG_TRACE("Byte 0x00, 0x01, or 0x03: 0x%02X\n", sig)

  /* Preview Address */
  dat->byte = 0x0D;
  preview_address = bit_read_RL(dat);
  LOG_TRACE("Preview Address: 0x%08X\n", (unsigned int) preview_address)

  /* DwgVer */
  dat->byte = 0x11;
  DwgVer = bit_read_RC(dat);
  LOG_INFO("DwgVer: %u\n", DwgVer)

  /* MaintReleaseVer */
  dat->byte = 0x12;
  MaintReleaseVer = bit_read_RC(dat);
  LOG_INFO("MaintRelease: %u\n", MaintReleaseVer)

  /* Codepage */
  dat->byte = 0x13;
  dwg->header.codepage = bit_read_RS(dat);
  LOG_TRACE("Codepage: %u\n", dwg->header.codepage)

  /* Unknown */
  dat->byte = 0x15;
  LOG_TRACE("Unknown: ")
  for (i = 0; i < 3; i++)
    {
      sig = bit_read_RC(dat);
      LOG_TRACE("0x%02X ", sig)
    }
  LOG_TRACE("\n")

  /* SecurityType */
  dat->byte = 0x18;
  security_type = bit_read_RL(dat);
  LOG_TRACE("SecurityType: 0x%08X\n", (unsigned int) security_type)

  /* Unknown long */
  dat->byte = 0x1C;
  unknown_long = bit_read_RL(dat);
  LOG_TRACE("Unknown long: 0x%08X\n", (unsigned int) unknown_long)

  /* DWG Property Addr */
  dat->byte = 0x20;
  dwg_property_address = bit_read_RL(dat);
  LOG_TRACE("DWG Property Addr: 0x%08X\n",
        (unsigned int) dwg_property_address)

  /* VBA Project Addr */
  dat->byte = 0x24;
  vba_proj_address = bit_read_RL(dat);
  LOG_TRACE("VBA Project Addr: 0x%08X\n",
        (unsigned int) vba_proj_address)

  /* 0x00000080 */
  dat->byte = 0x28;
  unknown_long = bit_read_RL(dat);
  LOG_TRACE("0x00000080: 0x%08X\n", (unsigned int) unknown_long)

  /* Application Info Address */
  dat->byte = 0x2C;
  app_info_address = bit_read_RL(dat);
  LOG_TRACE("Application Info Address: 0x%08X\n",
        (unsigned int) app_info_address)

  read_r2007_meta_data(dat, dwg);
  
  LOG_TRACE("\n\n")

  /////////////////////////////////////////
  //	incomplete implementation!
  /////////////////////////////////////////
  resolve_objectref_vector(dwg);

  LOG_ERROR(
      "Decoding of DWG version R2007 header is not fully implemented yet. we are going to try\n")
  return 0;
}

/*--------------------------------------------------------------------------------
 * Private functions
 */

int
dwg_decode_entity(Bit_Chain * dat, Dwg_Object_Entity * ent)
{
  unsigned int i;
  unsigned int size;
  int error = 2;

  SINCE(R_2000)
    {
      ent->bitsize = bit_read_RL(dat);
    }

  error = bit_read_H(dat, &(ent->object->handle));
  if (error)
    {
      LOG_ERROR(
          "dwg_decode_entity:\tError in object handle! Current Bit_Chain address: 0x%0x\n",
          (unsigned int) dat->byte)
      ent->bitsize = 0;
      ent->extended_size = 0;
      ent->picture_exists = 0;
      ent->num_handles = 0;
      return 0;
    }

  ent->extended_size = 0;
  while (size = bit_read_BS(dat))
    {
      LOG_TRACE("EED size: %lu\n", (long unsigned int)size)
      if (size > 10210)
        {
          LOG_ERROR(
              "dwg_decode_entity: Absurd! Extended object data size: %lu. Object: %lu (handle).\n",
              (long unsigned int) size, ent->object->handle.value)
          ent->bitsize = 0;
          ent->extended_size = 0;
          ent->picture_exists = 0;
          ent->num_handles = 0;
          //XXX
          return -1;
          //break;
        }
      if (ent->extended_size == 0)
        {
          ent->extended = (char *)malloc(size);
          ent->extended_size = size;
        }
      else
        {
          ent->extended_size += size;
          ent->extended = (char *)realloc(ent->extended, ent->extended_size);
        }
      error = bit_read_H(dat, &ent->extended_handle);
      if (error)
        LOG_ERROR("Error reading extended handle!\n");
      for (i = ent->extended_size - size; i < ent->extended_size; i++)
        ent->extended[i] = bit_read_RC(dat);
    }
  ent->picture_exists = bit_read_B(dat);
  if (ent->picture_exists)
    {
      ent->picture_size = bit_read_RL(dat);
      if (ent->picture_size < 210210)
        {
          ent->picture = (char *)malloc(ent->picture_size);
          for (i = 0; i < ent->picture_size; i++)
            ent->picture[i] = bit_read_RC(dat);
        }
      else
        {
          LOG_ERROR(
              "dwg_decode_entity:  Absurd! Picture-size: %lu kB. Object: %lu (handle).\n",
              ent->picture_size / 1000, ent->object->handle.value)
          bit_advance_position(dat, -(4 * 8 + 1));
        }
    }

  VERSIONS(R_13,R_14)
    {
      ent->bitsize = bit_read_RL(dat);
    }

  ent->entity_mode = bit_read_BB(dat);
  ent->num_reactors = bit_read_BL(dat);

  SINCE(R_2004)
    {
      ent->xdic_missing_flag = bit_read_B(dat);
    }

  VERSIONS(R_13,R_14)
    {
      ent->isbylayerlt = bit_read_B(dat);
    }

  ent->nolinks = bit_read_B(dat);

  SINCE(R_2004)
    {
      char color_mode = 0;
      unsigned char index;
      unsigned int flags;
    
      if (ent->nolinks == 0)
        {        
          color_mode = bit_read_B(dat);
        
          if (color_mode == 1)
            index = bit_read_RC(dat);  // color index
          else
            {              
              flags = bit_read_RS(dat);
            
              if (flags & 0x8000)
                {
                  unsigned char c1, c2, c3, c4;
                  char *name=0;
              
                  c1 = bit_read_RC(dat);  // rgb color
                  c2 = bit_read_RC(dat);
                  c3 = bit_read_RC(dat);
                  c4 = bit_read_RC(dat);
              
                  name = bit_read_TV(dat);
                }
            
              if (flags & 0x4000)
                flags = flags;   // has AcDbColor reference (handle)
            
              if (flags & 0x2000)
                {
                  int transparency = bit_read_BL(dat);
                }
            }
        }
      else
        {
          char color = bit_read_B(dat);
        }
    }
  OTHER_VERSIONS
    bit_read_CMC(dat, &ent->color);

  ent->linetype_scale = bit_read_BD(dat);

  SINCE(R_2000)
    {
      ent->linetype_flags = bit_read_BB(dat);
      ent->plotstyle_flags = bit_read_BB(dat);
    }

  SINCE(R_2007)
    {
      ent->material_flags = bit_read_BB(dat);
      ent->shadow_flags = bit_read_RC(dat);
    }

  ent->invisible = bit_read_BS(dat);

  SINCE(R_2000)
    {
      ent->lineweight = bit_read_RC(dat);
    }

  return 0;
}

int
dwg_decode_object(Bit_Chain * dat, Dwg_Object_Object * ord)
{
  unsigned int i;
  unsigned int size;
  int error = 2;

  SINCE(R_2000)
    {
      ord->bitsize = bit_read_RL(dat);
    }

  error = bit_read_H(dat, &ord->object->handle);
  if (error)
    {
      LOG_ERROR(
          "\tError in object handle! Bit_Chain current address: 0x%0x\n",
          (unsigned int) dat->byte)
      ord->bitsize = 0;
      ord->extended_size = 0;
      ord->num_handles = 0;
      return -1;
    }
  ord->extended_size = 0;
  while (size = bit_read_BS(dat))
    {
      if (size > 10210)
        {
          LOG_ERROR(
              "dwg_decode_object: Absurd! Extended object data size: %lu. Object: %lu (handle).\n",
              (long unsigned int) size, ord->object->handle.value)
          ord->bitsize = 0;
          ord->extended_size = 0;
          ord->num_handles = 0;
          return 0;
        }
      if (ord->extended_size == 0)
        {
          ord->extended = (unsigned char *)malloc(size);
          ord->extended_size = size;
        }
      else
        {
          ord->extended_size += size;
          ord->extended = (unsigned char *)realloc(ord->extended, ord->extended_size);
        }
      error = bit_read_H(dat, &ord->extended_handle);
      if (error)
        LOG_ERROR("Error reading extended handle!\n")
      for (i = ord->extended_size - size; i < ord->extended_size; i++)
        ord->extended[i] = bit_read_RC(dat);
    }

  VERSIONS(R_13,R_14)
    {
      ord->bitsize = bit_read_RL(dat);
    }

  ord->num_reactors = bit_read_BL(dat);

  SINCE(R_2004)
    {
      ord->xdic_missing_flag = bit_read_B(dat);
    }

  return 0;
}


Dwg_Object_Ref *
dwg_decode_handleref(Bit_Chain * dat, Dwg_Object * obj, Dwg_Data* dwg)
{
  // Welcome to the house of evil code!
  Dwg_Object_Ref* ref = (Dwg_Object_Ref *) malloc(sizeof(Dwg_Object_Ref));

  if (bit_read_H(dat, &ref->handleref))
    {
      if (obj)
        {
          LOG_ERROR(
            "Could not read handleref in object whose handle is: %d.%d.%lu\n",
            obj->handle.code, obj->handle.size, obj->handle.value)
        }
      else
        {
          LOG_ERROR("Could not read handleref in the header variables section\n")
        }
      free(ref);
      return 0;
    }

  //Reserve memory space for object references
  if (dwg->num_object_refs == 0)
    dwg->object_ref = (Dwg_Object_Ref **) malloc(REFS_PER_REALLOC * sizeof(Dwg_Object_Ref*));
  else
    if (dwg->num_object_refs % REFS_PER_REALLOC == 0)
      {
        dwg->object_ref = (Dwg_Object_Ref **) realloc(dwg->object_ref,
            (dwg->num_object_refs + REFS_PER_REALLOC) * sizeof(Dwg_Object_Ref*));
      }

  dwg->object_ref[dwg->num_object_refs++] = ref;

  ref->absolute_ref = ref->handleref.value;
  ref->obj = 0;

  //we receive a null obj when we are reading
  // handles in the header variables section
  if (!obj)
    return ref;

  /*
   * sometimes the code indicates the type of ownership
   * in other cases the handle is stored as an offset from some other handle
   * how is it determined?
   */
  ref->absolute_ref = 0;
  switch(ref->handleref.code) //that's right: don't bother the code on the spec.
    {
    case 0x06: //what if 6 means HARD_OWNER?
      ref->absolute_ref = (obj->handle.value + 1);
      break;
    case 0x08:
      ref->absolute_ref = (obj->handle.value - 1);
      break;
    case 0x0A:
      ref->absolute_ref = (obj->handle.value + ref->handleref.value);
      break;
    case 0x0C:
      ref->absolute_ref = (obj->handle.value - ref->handleref.value);
      break;
    default: //0x02, 0x03, 0x04, 0x05 or none
      ref->absolute_ref = ref->handleref.value;
      break;
    }
  return ref;
}

Dwg_Object_Ref *
dwg_decode_handleref_with_code(Bit_Chain * dat, Dwg_Object * obj, Dwg_Data* dwg, unsigned int code)
{
  Dwg_Object_Ref * ref;
  ref = dwg_decode_handleref(dat, obj, dwg);

  if (!ref)
    {
      LOG_ERROR("dwg_decode_handleref_with_code: ref is a null pointer\n");
      return 0;
    }

  if (ref->absolute_ref == 0 && ref->handleref.code != code)
    {
      LOG_ERROR("Expected a CODE %d handle, got a %d\n", code, ref->handleref.code)
      //TODO: At the moment we are tolerating wrong codes in handles.
      // in the future we might want to get strict and return 0 here so that code will crash
      // whenever it reaches the first handle parsing error. This might make debugging easier.
      //return 0;
    }
  return ref;
}

void
dwg_decode_common_entity_handle_data(Bit_Chain * dat, Dwg_Object * obj)
{

  //XXX setup required to use macros
  Dwg_Object_Entity *ent;
  Dwg_Data *dwg = obj->parent;
  int i;
  long unsigned int vcount;
  Dwg_Object_Entity *_obj;
  ent = obj->tio.entity;
  _obj = ent;

  #include "common_entity_handle_data.spec"

}

enum RES_BUF_VALUE_TYPE
{
  VT_INVALID = 0,
  VT_STRING = 1,
  VT_POINT3D = 2,
  VT_REAL = 3,
  VT_INT16 = 4,
  VT_INT32 = 5,
  VT_INT8 = 6,
  VT_BINARY = 7,
  VT_HANDLE = 8,
  VT_OBJECTID = 9,
  VT_BOOL = 10
};

enum RES_BUF_VALUE_TYPE
get_base_value_type(short gc)
{
  if (gc >= 300)
    {
      if (gc >= 440) 
        {
          if (gc >= 1000)  // 1000-1071
            {
              if (gc == 1004) return VT_BINARY;
              if (gc <= 1009) return VT_STRING;  
              if (gc <= 1059) return VT_REAL;
              if (gc <= 1070) return VT_INT16;
              if (gc == 1071) return VT_INT32;
            }
          else            // 440-999
            {
              if (gc <= 459) return VT_INT32;
              if (gc <= 469) return VT_REAL;
              if (gc <= 479) return VT_STRING;
              if (gc <= 998) return VT_INVALID;
              if (gc == 999) return VT_STRING;
            }         
        }
      else // <440 
        {
          if (gc >= 390)  // 390-439
            {
              if (gc <= 399) return VT_HANDLE;
              if (gc <= 409) return VT_INT16;
              if (gc <= 419) return VT_STRING;
              if (gc <= 429) return VT_INT32;
              if (gc <= 439) return VT_STRING;
            }
          else            // 330-389
            {
              if (gc <= 309) return VT_STRING;
              if (gc <= 319) return VT_BINARY;
              if (gc <= 329) return VT_HANDLE;
              if (gc <= 369) return VT_OBJECTID;
              if (gc <= 389) return VT_INT16;
            }
        }
    }
  else if (gc >= 105)
    {
      if (gc >= 210)      // 210-299
        {
          if (gc <= 239) return VT_REAL;
          if (gc <= 269) return VT_INVALID;
          if (gc <= 279) return VT_INT16;
          if (gc <= 289) return VT_INT8;
          if (gc <= 299) return VT_BOOL;
        }
      else               // 105-209
        {
          if (gc == 105) return VT_HANDLE;
          if (gc <= 109) return VT_INVALID;
          if (gc <= 149) return VT_REAL;
          if (gc <= 169) return VT_INVALID;
          if (gc <= 179) return VT_INT16;
          if (gc <= 209) return VT_INVALID;
        }
    }
  else  // <105
    {
      if (gc >= 38)     // 38-102
        {
          if (gc <= 59)  return VT_REAL;
          if (gc <= 79)  return VT_INT16;
          if (gc <= 99)  return VT_INT32;
          if (gc <= 101) return VT_STRING;
          if (gc == 102) return VT_STRING;
        }
      else              // 0-37
        {
          if (gc <= 0)   return VT_INVALID;
          if (gc <= 4)   return VT_STRING;
          if (gc == 5)   return VT_HANDLE;
          if (gc <= 9)   return VT_STRING;
          if (gc <= 37)  return VT_POINT3D;
        }
    }
  return VT_INVALID;
}

Dwg_Resbuf*
dwg_decode_xdata(Bit_Chain * dat, int size)
{
  char group_code;
  Dwg_Resbuf *rbuf, *root=0, *curr=0;
  unsigned char codepage;
  long unsigned int end_address;
  char hdl[8];
  int i, length;

  static int cnt = 0;
  cnt++;

  end_address = dat->byte + (unsigned long int)size;

  while (dat->byte < end_address)
    {
      rbuf = (Dwg_Resbuf *) malloc(sizeof(Dwg_Resbuf));
      rbuf->next = 0;
      rbuf->type = bit_read_RS(dat);

      switch (get_base_value_type(rbuf->type))
        {
        case VT_STRING:
          length   = bit_read_RS(dat);          
          codepage = bit_read_RC(dat);
          if (length > 0)
            {
              rbuf->value.str = (char *)malloc((length + 1) * sizeof(char));
              for (i = 0; i < length; i++)
                rbuf->value.str[i] = bit_read_RC(dat);
              rbuf->value.str[i] = '\0';
            }
          break;
        case VT_REAL:
          rbuf->value.dbl = bit_read_RD(dat);
          break;
        case VT_BOOL:
        case VT_INT8:
          rbuf->value.i8 = bit_read_RC(dat);
          break;
        case VT_INT16:
          rbuf->value.i16 = bit_read_RS(dat);
          break;
        case VT_INT32:
          rbuf->value.i32 = bit_read_RL(dat);
          break;
        case VT_POINT3D:
          rbuf->value.pt[0] = bit_read_RD(dat);
          rbuf->value.pt[1] = bit_read_RD(dat);
          rbuf->value.pt[2] = bit_read_RD(dat);
          break;
        case VT_BINARY:
          rbuf->value.chunk.size = bit_read_RC(dat);          
          if (rbuf->value.chunk.size > 0)
            {
              rbuf->value.chunk.data = (char *)malloc(rbuf->value.chunk.size * sizeof(char));
              for (i = 0; i < rbuf->value.chunk.size; i++)
                rbuf->value.chunk.data[i] = bit_read_RC(dat);
            }
          break;
        case VT_HANDLE:
        case VT_OBJECTID:
          for (i = 0; i < 8; i++)
             rbuf->value.hdl[i] = bit_read_RC(dat);
          break;
        default:
          LOG_ERROR("Invalid group code in xdata: %d!\n", rbuf->type)
          free(rbuf);
          dat->byte = end_address;
          return root;
          break;
        }

      if (curr == 0)
        curr = root = rbuf;
      else
        {
          curr->next = rbuf;
          curr = rbuf;
        }
    }
    return root;
}
