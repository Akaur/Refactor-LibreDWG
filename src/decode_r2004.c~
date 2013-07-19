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
 * decode_r2004.c: R2004 decoding functions
 * written by Felipe Castro
 * modified by Felipe Corrêa da Silva Sances
 * modified by Rodrigo Rodrigues da Silva
 * modified by Till Heuschmann
 */

#include <stdint.h>
#include <sys/types.h>

#include "decode_r2004.h"
#include "classes.h"
#include "decode_r13_r15.h"
#include "handle.h"
#include "header.h"
#include "logging.h"
#include "resolve_pointers.h"
#include "section_locate.h"

static unsigned int loglevel;

#define DWG_LOGLEVEL loglevel

/* Encrypted Section Header */
typedef union _encrypted_section_header
{
  int32_t long_data[8];
  unsigned char char_data[32];
  struct
  {
    int32_t tag;
    int32_t section_type;
    int32_t data_size;
    int32_t section_size;
    int32_t start_offset;
    int32_t unknown;
    int32_t checksum_1;
    int32_t checksum_2;
  } fields;
} encrypted_section_header;

/* R2004 Literal Length
 */
static int
read_literal_length(Bit_Chain* dat, unsigned char *opcode)
{
  int total = 0;
  unsigned char byte = bit_read_RC(dat);

  *opcode = 0x00;

  if (byte >= 0x01 && byte <= 0x0F)
    return byte + 3;
  else if (byte == 0)
    {
      total = 0x0F;
      while ((byte = bit_read_RC(dat)) == 0x00)
        {
          total += 0xFF;
        }
      return total + byte + 3;
    }
  else if (byte & 0xF0)
    *opcode = byte;

  return 0;
}

/* R2004 Long Compression Offset
 */
static int
read_long_compression_offset(Bit_Chain* dat)
{
  int total = 0;
  unsigned char byte = bit_read_RC(dat);
  if (byte == 0)
    {
      total = 0xFF;
      while ((byte = bit_read_RC(dat)) == 0x00)
        total += 0xFF;
	}
  return total + byte;
}

/* R2004 Two Byte Offset
 */
static int
read_two_byte_offset(Bit_Chain* dat, int* lit_length)
{
  int offset;
  unsigned char firstByte = bit_read_RC(dat);
  unsigned char secondByte = bit_read_RC(dat);
  offset = (firstByte >> 2) | (secondByte << 6);
  *lit_length = (firstByte & 0x03);
  return offset;
}

/* Decompresses a system section of a 2004 DWG flie
 */
static int
decompress_R2004_section(Bit_Chain* dat, char *decomp,
                         unsigned long int comp_data_size)
{
  int lit_length, i;
  int comp_offset, comp_bytes;
  unsigned char opcode1 = 0, opcode2;
  int32_t start_byte = dat->byte;
  char *src, *dst = decomp;

  // length of the first sequence of uncompressed or literal data.
  lit_length = read_literal_length(dat, &opcode1);

  for (i = 0; i < lit_length; ++i)
    *dst++ = bit_read_RC(dat);

  opcode1 = 0x00;
  while (dat->byte - start_byte < comp_data_size)
    {
      if (opcode1 == 0x00)
        opcode1 = bit_read_RC(dat);

      if (opcode1 >= 0x40)
        {
          comp_bytes = ((opcode1 & 0xF0) >> 4) - 1;
          opcode2 = bit_read_RC(dat);
          comp_offset = (opcode2 << 2) | ((opcode1 & 0x0C) >> 2);

          if (opcode1 & 0x03)
            {
              lit_length = (opcode1 & 0x03);
              opcode1  = 0x00;
            }
          else
            lit_length = read_literal_length(dat, &opcode1);
        }
      else if (opcode1 >= 0x21 && opcode1 <= 0x3F)
        {
          comp_bytes  = opcode1 - 0x1E;
          comp_offset = read_two_byte_offset(dat, &lit_length);

          if (lit_length != 0)
            opcode1 = 0x00;
          else
            lit_length = read_literal_length(dat, &opcode1);
        }
      else if (opcode1 == 0x20)
        {
          comp_bytes  = read_long_compression_offset(dat) + 0x21;
          comp_offset = read_two_byte_offset(dat, &lit_length);

          if (lit_length != 0)
            opcode1 = 0x00;
          else
            lit_length = read_literal_length(dat, &opcode1);
        }
      else if (opcode1 >= 0x12 && opcode1 <= 0x1F)
        {
          comp_bytes  = (opcode1 & 0x0F) + 2;
          comp_offset = read_two_byte_offset(dat, &lit_length) + 0x3FFF;

          if (lit_length != 0)
            opcode1 = 0x00;
          else
            lit_length = read_literal_length(dat, &opcode1);
        }
      else if (opcode1 == 0x10)
        {
          comp_bytes  = read_long_compression_offset(dat) + 9;
          comp_offset = read_two_byte_offset(dat, &lit_length) + 0x3FFF;

          if (lit_length != 0)
            opcode1 = 0x00;
          else
            lit_length = read_literal_length(dat, &opcode1);
        }
      else if (opcode1 == 0x11)
          break;     // Terminates the input stream, everything is ok!
      else
          return 1;  // error in input stream

      //LOG_TRACE("got compressed data %d\n",comp_bytes)
      // copy "compressed data"
      src = dst - comp_offset - 1;
      assert(src > decomp);
      for (i = 0; i < comp_bytes; ++i)
        *dst++ = *src++;

      // copy "literal data"
      //LOG_TRACE("got literal data %d\n",lit_length)
      for (i = 0; i < lit_length; ++i)
        *dst++ = bit_read_RC(dat);
    }

  return 0;  // Success
}

static Dwg_Section*
find_section(Dwg_Data *dwg, unsigned long int index)
{
  int i;
  if (dwg->header.section == 0 || index == 0)
    return 0;
  for (i = 0; i < dwg->header.num_sections; ++i)
    {
      if (dwg->header.section[i].number == index)
        return (&dwg->header.section[i]);
    }
  return 0;
}

/* Read R2004 Section Info
 */
static void
read_R2004_section_info(Bit_Chain* dat, Dwg_Data *dwg,
                        unsigned long int comp_data_size,
                        unsigned long int decomp_data_size)
{
  char *decomp, *ptr;
  int i, j;
  int section_number;
  int data_size;
  int start_offset;
  int unknown;

  decomp = (char *)malloc(decomp_data_size * sizeof(char));
  if (decomp == 0)
    return;   // No memory

  decompress_R2004_section(dat, decomp, comp_data_size);

  memcpy(&dwg->header.num_descriptions, decomp, 4);
  dwg->header.section_info = (Dwg_Section_Info*)
    malloc(sizeof(Dwg_Section_Info) * dwg->header.num_descriptions);

  LOG_TRACE("\n#### 2004 Section Info fields ####\n")
  LOG_TRACE("NumDescriptions: %d\n", *((int*)decomp))
  LOG_TRACE("0x02:            %x\n", *((int*)decomp + 1))
  LOG_TRACE("0x00007400:      %x\n", *((int*)decomp + 2))
  LOG_TRACE("0x00:            %x\n", *((int*)decomp + 3))
  LOG_TRACE("Unknown:         %x\n", *((int*)decomp + 4))

  ptr = decomp + 20;
  for (i = 0; i < dwg->header.num_descriptions; ++i)
    {
      dwg->header.section_info[i].size            = *((int*)ptr);
      dwg->header.section_info[i].unknown1 	      = *((int*)ptr + 1);
      dwg->header.section_info[i].num_sections    = *((int*)ptr + 2);
      dwg->header.section_info[i].max_decomp_size = *((int*)ptr + 3);
      dwg->header.section_info[i].unknown2        = *((int*)ptr + 4);
      dwg->header.section_info[i].compressed      = *((int*)ptr + 5);
      dwg->header.section_info[i].type            = *((int*)ptr + 6);
      dwg->header.section_info[i].encrypted       = *((int*)ptr + 7);
      ptr += 32;
      memcpy(dwg->header.section_info[i].name, ptr, 64);
      ptr += 64;

      LOG_TRACE("\nSection Info description fields\n")
      LOG_TRACE("Size:                  %d",
           (int) dwg->header.section_info[i].size)
      LOG_TRACE("Unknown:               %d\n",
           (int) dwg->header.section_info[i].unknown1)
      LOG_TRACE("Number of sections:    %d\n",
           (int) dwg->header.section_info[i].num_sections)
      LOG_TRACE("Max decompressed size: %d\n",
           (int) dwg->header.section_info[i].max_decomp_size)
      LOG_TRACE("Unknown:               %d\n",
           (int) dwg->header.section_info[i].unknown2)
      LOG_TRACE("Compressed (0x02):     %x\n",
           (unsigned int) dwg->header.section_info[i].compressed)
      LOG_TRACE("Section Type:          %d\n",
           (int) dwg->header.section_info[i].type)
      LOG_TRACE("Encrypted:             %d\n",
           (int) dwg->header.section_info[i].encrypted)
      LOG_TRACE("SectionName:           %s\n\n",
            dwg->header.section_info[i].name)

      dwg->header.section_info[i].sections = (Dwg_Section**)
        malloc(dwg->header.section_info[i].num_sections * sizeof(Dwg_Section*));

      if (dwg->header.section_info[i].num_sections < 10000)
	{
	  LOG_INFO("section count %ld in area %d\n",dwg->header.section_info[i].num_sections,i)

	  for (j = 0; j < dwg->header.section_info[i].num_sections; j++)
	    {
	      section_number = *((int*)ptr);      // Index into SectionMap
	      data_size      = *((int*)ptr + 1);
	      start_offset   = *((int*)ptr + 2);
	      unknown        = *((int*)ptr + 3);  // high 32 bits of 64-bit start offset?
	      ptr += 16;

	      dwg->header.section_info[i].sections[j] = find_section(dwg, section_number);

	      LOG_TRACE("Section Number: %d\n", section_number)
	      LOG_TRACE("Data size:      %d\n", data_size)
	      LOG_TRACE("Start offset:   %x\n", start_offset)
	      LOG_TRACE("Unknown:        %d\n", unknown)
	    }
	}// sanity check
      else
	{
	  LOG_ERROR("section count %ld in area %d too high! skipping\n",dwg->header.section_info[i].num_sections,i)
	}
    }
  free(decomp);
}


static int
read_2004_compressed_section(Bit_Chain* dat, Dwg_Data *dwg,
                            Bit_Chain* sec_dat,
                            long unsigned int section_type)
{
  int32_t address, sec_mask;
  int32_t max_decomp_size;
  Dwg_Section_Info *info = 0;
  encrypted_section_header es;
  char *decomp;
  int i, j;

  for (i = 0; i < dwg->header.num_descriptions && info == 0; ++i)
    if (dwg->header.section_info[i].type == section_type)
      info = &dwg->header.section_info[i];

  if (info == 0)
    return 1;   // Failed to find section

  max_decomp_size = info->num_sections * info->max_decomp_size;

  decomp = (char *)malloc(max_decomp_size * sizeof(char));
  if (decomp == 0)
    return 2;   // No memory

  for (i = 0; i < info->num_sections; ++i)
    {
      address = info->sections[i]->address;
      dat->byte = address;

      for (j = 0; j < 0x20; j++)
        es.char_data[j] = bit_read_RC(dat);

      sec_mask = 0x4164536b ^ address;
      for (j = 0; j < 8; ++j)
        es.long_data[j] ^= sec_mask;

  LOG_INFO("\n=== Section (Class) ===\n")
  LOG_INFO("Section Tag (should be 0x4163043b): %x\n",
          (unsigned int) es.fields.tag)
  LOG_INFO("Section Type:     %x\n",
          (unsigned int) es.fields.section_type)
  LOG_INFO("Data size:   %x\n",
          (unsigned int) es.fields.data_size)   // this is the number of bytes that is read in decompress_R2004_section (+ 2bytes)
  LOG_INFO("Comp data size:     %x\n",
          (unsigned int) es.fields.section_size)
  LOG_INFO("StartOffset:      %x\n",
          (unsigned int) es.fields.start_offset)
  LOG_INFO("Unknown:          %x\n",
          (unsigned int) es.fields.unknown);
  LOG_INFO("Checksum1:        %x\n",
        (unsigned int) es.fields.checksum_1)
  LOG_INFO("Checksum2:        %x\n\n",
        (unsigned int) es.fields.checksum_2)

      decompress_R2004_section(dat, &decomp[i * info->max_decomp_size],
        es.fields.data_size);
    }

  sec_dat->bit     = 0;
  sec_dat->byte    = 0;
  sec_dat->chain   = (unsigned char *)decomp;
  sec_dat->size    = max_decomp_size;
  sec_dat->version = dat->version;

  return 0;
}

int
decode_R2004(Bit_Chain* dat, Dwg_Data * dwg)
{
  /* Encrypted Data */
  union
  {
    unsigned char encrypted_data[0x6c];
    struct
    {
      unsigned char file_ID_string[12];
      int32_t x00;
      int32_t x6c;
      int32_t x04;
      int32_t root_tree_node_gap;
      int32_t lowermost_left_tree_node_gap;
      int32_t lowermost_right_tree_node_gap;
      int32_t unknown_long;
      int32_t last_section_id;
      int32_t last_section_address;
      int32_t x00_2;
      int32_t second_header_address;
      int32_t x00_3;
      int32_t gap_amount;
      int32_t section_amount;
      int32_t x20;
      int32_t x80;
      int32_t x40;
      int32_t section_map_id;
      int32_t section_map_address;
      int32_t x00_4;
      int32_t section_info_id;
      int32_t section_array_size;
      int32_t gap_array_size;
      int32_t CRC;
    } fields;
  } _2004_header_data;

  /* System Section */
  typedef union _system_section
  {
    unsigned char data[0x14];
    struct
    {
      int32_t section_type;   //0x4163043b
      int32_t decomp_data_size;
      int32_t comp_data_size;
      int32_t compression_type;
      int32_t checksum;
    } fields;
  } system_section;

  system_section ss;
  int rseed = 1;

  Dwg_Section *section;

  int i;
  int32_t preview_address, security_type, unknown_long,
          dwg_property_address, vba_proj_address;
  unsigned char sig, dwg_ver, maint_release_ver;

  //6 bytes of 0x00
  dat->byte = 0x06;
  LOG_INFO("6 bytes of 0x00: ")
  for (i = 0; i < 6; i++)
    {
      sig = bit_read_RC(dat);
      LOG_INFO( "0x%02X ", sig)
    }
  LOG_INFO("\n")

  /* Byte 0x00, 0x01, or 0x03 */
  dat->byte = 0x0C;
  sig = bit_read_RC(dat);
  LOG_INFO("Byte 0x00, 0x01, or 0x03: 0x%02X\n", sig)

  /* Preview Address */
  dat->byte = 0x0D;
  preview_address = bit_read_RL(dat);
  LOG_INFO("Preview Address: 0x%08X\n", (unsigned int) preview_address)

  /* DwgVer */
  dat->byte = 0x11;
  dwg_ver = bit_read_RC(dat);
  LOG_INFO("DwgVer: %u\n", dwg_ver)

  /* MaintReleaseVer */
  dat->byte = 0x12;
  maint_release_ver = bit_read_RC(dat);
  LOG_INFO("MaintRelease: %u\n", maint_release_ver)

  /* Codepage */
  dat->byte = 0x13;
  dwg->header.codepage = bit_read_RS(dat);
  LOG_INFO("Codepage: %u\n", dwg->header.codepage)

  /* 3 0x00 bytes */
  dat->byte = 0x15;
  LOG_INFO("3 0x00 bytes: ")
  for (i = 0; i < 3; i++)
    {
      sig = bit_read_RC(dat);
      LOG_INFO("0x%02X ", sig)
    }
  LOG_INFO("\n")

  /* SecurityType */
  dat->byte = 0x18;
  security_type = bit_read_RL(dat);
  LOG_INFO("SecurityType: 0x%08X\n", (unsigned int) security_type)

  /* Unknown long */
  dat->byte = 0x1C;
  unknown_long = bit_read_RL(dat);
  LOG_INFO("Unknown long: 0x%08X\n", (unsigned int) unknown_long)

  /* DWG Property Addr */
  dat->byte = 0x20;
  dwg_property_address = bit_read_RL(dat);
  LOG_INFO("DWG Property Addr: 0x%08X\n",
        (unsigned int) dwg_property_address)

  /* VBA Project Addr */
  dat->byte = 0x24;
  vba_proj_address = bit_read_RL(dat);
  LOG_INFO("VBA Project Addr: 0x%08X\n",
        (unsigned int) vba_proj_address)

  /* 0x00000080 */
  dat->byte = 0x28;
  unknown_long = bit_read_RL(dat);
  LOG_INFO("0x00000080: 0x%08X\n", (unsigned int) unknown_long)

  /* 0x00 bytes (length = 0x54 bytes) */
  dat->byte = 0x2C;
  for (i = 0; i < 0x54; i++)
    {
      sig = bit_read_RC(dat);
      if (sig != 0 && loglevel)
        LOG_ERROR(
            "Warning: Byte should be zero! But a value=%x was read instead.\n",
            sig)
    }

  dat->byte = 0x80;
  for (i = 0; i < 0x6c; i++)
    {
      rseed *= 0x343fd;
      rseed += 0x269ec3;
      _2004_header_data.encrypted_data[i] = bit_read_RC(dat) ^ (rseed >> 0x10);
    }

  if (loglevel)
    {
      LOG_TRACE("\n#### 2004 File Header Data fields ####\n")
      LOG_TRACE("File ID string (must be AcFssFcAJMB): ");
      for (i = 0; i < 12; i++)
        {
          LOG_TRACE("%c", _2004_header_data.fields.file_ID_string[i])
        }
      LOG_TRACE("\n")
      LOG_TRACE("0x00 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x00)
      LOG_TRACE("0x6c (long): %x\n",
          (unsigned int) _2004_header_data.fields.x6c)
      LOG_TRACE("0x04 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x04)
      LOG_TRACE("Root tree node gap: %x\n",
          (unsigned int) _2004_header_data.fields.root_tree_node_gap)
      LOG_TRACE("Lowermost left tree node gap: %x\n",
          (unsigned int) _2004_header_data.fields.lowermost_left_tree_node_gap)
      LOG_TRACE("Lowermost right tree node gap: %x\n",
          (unsigned int) _2004_header_data.fields.lowermost_right_tree_node_gap)
      LOG_TRACE("Unknown long: %x\n",
          (unsigned int) _2004_header_data.fields.unknown_long)
      LOG_TRACE("Last section id: %x\n",
          (unsigned int) _2004_header_data.fields.last_section_id)
      LOG_TRACE("Last section address: %x\n",
          (unsigned int) _2004_header_data.fields.last_section_address)
      LOG_TRACE("0x00 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x00_2)
      LOG_TRACE("Second header address: %x\n",
          (unsigned int) _2004_header_data.fields.second_header_address)
      LOG_TRACE("0x00 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x00_3)
      LOG_TRACE("Gap amount: %x\n",
          (unsigned int) _2004_header_data.fields.gap_amount)
      LOG_TRACE("Section amount: %x\n",
          (unsigned int) _2004_header_data.fields.section_amount)
      LOG_TRACE("0x20 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x20)
      LOG_TRACE("0x80 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x80)
      LOG_TRACE("0x40 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x40)
      LOG_TRACE("Section map id: %x\n",
          (unsigned int) _2004_header_data.fields.section_map_id)
      LOG_TRACE("Section map address: %x\n",
          (unsigned int) _2004_header_data.fields.section_map_address + 0x100)
      LOG_TRACE("0x00 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x00_4)
      LOG_TRACE("Section Info id: %x\n",
          (unsigned int) _2004_header_data.fields.section_info_id)
      LOG_TRACE("Section array size: %x\n",
          (unsigned int) _2004_header_data.fields.section_array_size)
      LOG_TRACE("Gap array size: %x\n",
          (unsigned int) _2004_header_data.fields.gap_array_size)
      LOG_TRACE("CRC: %x\n", (unsigned int) _2004_header_data.fields.CRC)
    }

  /*-------------------------------------------------------------------------
   * Section Map
   */
  dat->byte = _2004_header_data.fields.section_map_address + 0x100;

  LOG_TRACE("\n\nRaw system section bytes:\n");
  for (i = 0; i < 0x14; i++)
    {
      ss.data[i] = bit_read_RC(dat);
      LOG_TRACE("%x ", ss.data[i])
    }

  LOG_TRACE("\n=== System Section (Section Map) ===\n")
  LOG_TRACE("Section Type (should be 0x4163043b): %x\n",
          (unsigned int) ss.fields.section_type)
  LOG_TRACE("DecompDataSize: %x\n",
          (unsigned int) ss.fields.decomp_data_size)
  LOG_TRACE("CompDataSize: %x\n",
          (unsigned int) ss.fields.comp_data_size)
  LOG_TRACE("Compression Type: %x\n",
          (unsigned int) ss.fields.compression_type)
  LOG_TRACE("Checksum: %x\n\n", (unsigned int) ss.fields.checksum)

  read_R2004_section_map(dat, dwg,
      ss.fields.comp_data_size, ss.fields.decomp_data_size);

  if (dwg->header.section == 0)
    {
      LOG_ERROR("Failed to read R2004 Section Map.\n")
      return -1;
    }

  /*-------------------------------------------------------------------------
   * Section Info
   */
  section = find_section(dwg, _2004_header_data.fields.section_info_id);

  if (section != 0)
    {
      dat->byte = section->address;
      LOG_TRACE("\nRaw system section bytes:\n")
      for (i = 0; i < 0x14; i++)
        {
          ss.data[i] = bit_read_RC(dat);
          LOG_TRACE("%x ", ss.data[i])
        }

      LOG_TRACE("\n=== System Section (Section Info) ===\n")
      LOG_TRACE("Section Type (should be 0x4163043b): %x\n",
              (unsigned int) ss.fields.section_type)
      LOG_TRACE("DecompDataSize: %x\n",
              (unsigned int) ss.fields.decomp_data_size)
      LOG_TRACE("CompDataSize: %x\n",
              (unsigned int) ss.fields.comp_data_size)
      LOG_TRACE("Compression Type: %x\n",
              (unsigned int) ss.fields.compression_type)
      LOG_TRACE("Checksum: %x\n\n", (unsigned int) ss.fields.checksum)

       read_R2004_section_info(dat, dwg,
         ss.fields.comp_data_size, ss.fields.decomp_data_size);
    }

  read_2004_section_classes(dat, dwg);
  read_2004_section_header(dat, dwg);
  read_2004_section_handles(dat, dwg);

  /* Clean up */
  if (dwg->header.section_info != 0)
    {
      for (i = 0; i < dwg->header.num_descriptions; ++i)
        if (dwg->header.section_info[i].sections != 0)
          free(dwg->header.section_info[i].sections);

      free(dwg->header.section_info);
      dwg->header.num_descriptions = 0;
    }

  resolve_objectref_vector(dwg);

  LOG_ERROR(
	  "Decoding of DWG version R2004 header is not fully implemented yet. We are going to try\n")
  return 0;
}
