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
 * handle.c: handle section functions
 * written by Felipe Castro
 * modified by Felipe Corrêa da Silva Sances
 * modified by Rodrigo Rodrigues da Silva
 * modified by Till Heuschmann
 */

#include "handle.h"
#include "object.h"
#include "decode_r2004.h"

extern unsigned int
bit_ckr8(unsigned int dx, unsigned char *adr, long n);

/* Read R13-R15 Object-map Section
 */
static void
read_R13_R15_section_object_map(Bit_Chain *dat, Dwg_Data *dwg)
{
  unsigned int section_size = 0;
  unsigned char sgdc[2];
  unsigned int ckr, ckr2, antckr;
  long unsigned int size;
  long unsigned int maplasta;
  long unsigned int duabyte;
  long unsigned int object_begin;
  long unsigned int object_end;

  dat->byte = dwg->header.section[2].address;
  dat->bit = 0;

  maplasta = dat->byte + dwg->header.section[2].size; // 4
  dwg->num_objects = 0;
  object_begin = dat->size;
  object_end = 0;
  do
    {
      long unsigned int last_address;
      long unsigned int last_handle;
      long unsigned int previous_address = 0;

      duabyte = dat->byte;
      sgdc[0] = bit_read_RC(dat);
      sgdc[1] = bit_read_RC(dat);
      section_size = (sgdc[0] << 8) | sgdc[1];
      //LOG_TRACE("section_size: %u\n", section_size)
 
      if (section_size > 2035)
        {
          LOG_ERROR("Object-map section size greater than 2035!\n")
          return -1;
        }

      last_handle = 0;
      last_address = 0;

      while (dat->byte - duabyte < section_size)
        {
          long unsigned int kobj;
          long int pvztkt;
          long int pvzadr;

          previous_address = dat->byte;
          pvztkt = bit_read_MC(dat);
          last_handle += pvztkt;
          pvzadr = bit_read_MC(dat);
          last_address += pvzadr;

          // LOG_TRACE("Idc: %li\t", dwg->num_objects)
          // LOG_TRACE("Handle: %li\tAddress: %li", pvztkt, pvzadr)
          //}

          if (dat->byte == previous_address)
            break;

          //if (dat->byte - duabyte >= seksize)
          //break;

          if (object_end < last_address)
            object_end = last_address;

          if (object_begin > last_address)
            object_begin = last_address;

          kobj = dwg->num_objects;
          dwg_decode_add_object(dwg, dat, last_address);

          //if (dwg->num_objects > kobj)
          //dwg->object[dwg->num_objects - 1].handle.value = lastahandle;
          //TODO: blame Juca
        }
      if (dat->byte == previous_address)
        break;

      // CRC on
      if (dat->bit > 0)
        {
          dat->byte += 1;
          dat->bit = 0;
        }

      sgdc[0] = bit_read_RC(dat);
      sgdc[1] = bit_read_RC(dat);
      ckr = (sgdc[0] << 8) | sgdc[1];

      ckr2 = bit_ckr8(0xc0c1, dat->chain + duabyte, section_size);

      if (ckr != ckr2)
        {
          printf("section %d crc todo ckr:%x ckr2:%x\n",
                  dwg->header.section[2].number, ckr, ckr2);
          return -1;
        }

      if (dat->byte >= maplasta)
        break;
    }
  while (section_size > 2);

  LOG_INFO("Num objects: %lu\n", dwg->num_objects)
  LOG_INFO("\n Object Data: %8X\n", (unsigned int) object_begin)
  dat->byte = object_end;
  object_begin = bit_read_MS(dat);

  LOG_INFO(" Object Data (end): %8X\n",
           (unsigned int) (object_end + object_begin+ 2))

  /*
   dat->byte = dwg->header.section[2].address - 2;
   antckr = bit_read_CRC (dat);// Unknown bitdouble inter object data and objectmap
   LOG_TRACE("Address: %08X / Content: 0x%04X", dat->byte - 2, antckr)

   // check CRC-on
   antckr = 0xC0C1;
   do
   {
   duabyte = dat->byte;
   sgdc[0] = bit_read_RC (dat);
   sgdc[1] = bit_read_RC (dat);
   section_size = (sgdc[0] << 8) | sgdc[1];
   section_size -= 2;
   dat->byte += section_size;
   ckr = bit_read_CRC (dat);
   dat->byte -= 2;
   bit_write_CRC (dat, duabyte, antckr);
   dat->byte -= 2;
   ckr2 = bit_read_CRC (dat);
   if (loglevel) fprintf (stderr, "Read: %X\nCreated: %X\t SEMO: %X\n", ckr, ckr2, antckr);
   //antckr = ckr;
   } while (section_size > 0);
   */

  LOG_INFO("\n=======> Object Map: %8X\n",
           (unsigned int) dwg->header.section[2].address)
  LOG_INFO("         Object Map (end): %8X\n",
           (unsigned int) (dwg->header.section[2].address
              + dwg->header.section[2].size))
}

/* R2004 Handles Section
 */
static void
read_2004_section_handles(Bit_Chain *dat, Dwg_Data *dwg)
{
  unsigned int section_size = 0;
  unsigned char sgdc[2];
  long unsigned int duabyte;
  long unsigned int maplasta;
  Bit_Chain hdl_dat;
  Bit_Chain obj_dat;

  if (read_2004_compressed_section(dat, dwg, &obj_dat, 
      SECTION_DBOBJECTS) != 0)
    return;

  if (read_2004_compressed_section(dat, dwg, &hdl_dat, 
      SECTION_HANDLES) != 0)
    {
      free(obj_dat.chain);
      return;
    }

  maplasta = hdl_dat.byte + hdl_dat.size;
  dwg->num_objects = 0;

  do
    {
      long unsigned int last_offset;
      long unsigned int last_handle;
      long unsigned int previous_address = 0;

      duabyte = hdl_dat.byte;
      sgdc[0] = bit_read_RC(&hdl_dat);
      sgdc[1] = bit_read_RC(&hdl_dat);
      section_size = (sgdc[0] << 8) | sgdc[1];

      LOG_TRACE("section_size: %u\n", section_size);

      if (section_size > 2034)
        LOG_INFO("Error: Object-map section size greater than 2034!\n");

      last_handle = 0;
      last_offset = 0;
      while (hdl_dat.byte - duabyte < section_size)
        {
          long int pvztkt;
          long int pvzadr;

          previous_address = hdl_dat.byte;

          pvztkt = bit_read_MC(&hdl_dat);
          last_handle += pvztkt;

          pvzadr = bit_read_MC(&hdl_dat);
          last_offset += pvzadr;

          dwg_decode_add_object(dwg, &obj_dat, last_offset);
        }

      if (hdl_dat.byte == previous_address)
        break;
      hdl_dat.byte += 2; // CRC

      if (hdl_dat.byte >= maplasta)
        break;
    }
  while (section_size > 2);

  LOG_TRACE("\nNum objects: %lu\n", dwg->num_objects);

  free(hdl_dat.chain);
  free(obj_dat.chain);
}


