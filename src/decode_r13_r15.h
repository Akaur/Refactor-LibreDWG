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
 * decode_r13_r15.h: R13_R15 decoding functions
 * written by Felipe Castro
 * modified by Felipe Corrêa da Silva Sances
 * modified by Rodrigo Rodrigues da Silva
 * modified by Till Heuschmann
 */

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

int
decode_R13_R15(Bit_Chain *dat, Dwg_Data *dwg);

void
read_section_unknown(Bit_Chain *dat, Dwg_Data *dwg);

void
read_section_picture(Bit_Chain *dat, Dwg_Data *dwg);

void
read_section_second_header(Bit_Chain *dat, Dwg_Data *dwg);

void
read_section_measurement(Bit_Chain *dat, Dwg_Data *dwg);

#endif
