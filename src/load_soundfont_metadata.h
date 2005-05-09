/* FluidSynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2005 Sean Bolton and others.
 *
 * All of this code comes from FluidSynth, copyright (C) 2003 Peter
 * Hanappe and others, which in turn borrows code from GLIB,
 * copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh
 * MacDonald, and from Smurf SoundFont Editor, copyright (C) Josh
 * Green.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _LOAD_SFONT_METADATA_H
#define _LOAD_SFONT_METADATA_H

#include <stdio.h>

/* ==== from fluid_list.h: ==== */

typedef struct _fluid_list_t fluid_list_t;

struct _fluid_list_t
{
  void* data;
  fluid_list_t *next;
};

#define fluid_list_next(slist)	((slist) ? (((fluid_list_t *)(slist))->next) : NULL)

/* ==== from fluid_defsfont.h: ==== */

/* Sound Font structure defines */

typedef struct _SFVersion
{                               /* version structure */
      unsigned short major;
      unsigned short minor;
}
SFVersion;

typedef struct _SFPreset
{                               /* Preset structure */
      char name[21];            /* preset name */
      unsigned short prenum;    /* preset number */
      unsigned short bank;      /* bank number */
      unsigned int libr;        /* Not used (preserved) */
      unsigned int genre;       /* Not used (preserved) */
      unsigned int morph;       /* Not used (preserved) */
      fluid_list_t *zone;       /* list of preset zones */
}
SFPreset;

typedef struct _SFData
{                               /* Sound font data structure */
  SFVersion version;            /* sound font version */
  SFVersion romver;             /* ROM version */
  unsigned int samplepos;       /* position within sffd of the sample chunk */
  unsigned int samplesize;      /* length within sffd of the sample chunk */
  char *fname;                  /* file name */
  FILE *sffd;                   /* loaded sfont file descriptor */
  fluid_list_t *info;           /* linked list of info strings (1st byte is ID) */
  fluid_list_t *preset;         /* linked list of preset info */
  fluid_list_t *inst;           /* linked list of instrument info */
  fluid_list_t *sample;         /* linked list of sample info */
}
SFData;

SFData *fsd_sfload_file (const char * fname);
void    fsd_sfont_free_data (SFData * sf);

#endif /* _LOAD_SFONT_METADATA_H */
