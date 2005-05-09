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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "load_soundfont_metadata.h"

#define _(X)  (X)
#define FLUID_FREE(_p)  free(_p)
#define FLUID_LOG(E, X...)  fprintf(stderr, X)
#define FLUID_MALLOC(_n)  malloc(_n)
#define FLUID_NEW(_t)  (_t*)malloc(sizeof(_t))
#define FLUID_STRDUP(s)  strdup(s)

/* ==== from fluidsynth/sfont.h: ==== */

#define FLUID_SAMPLETYPE_ROM	0x8000

/* ==== from fluid_list.h: ==== */

typedef int (*fluid_compare_func_t)(void* a, void* b);

fluid_list_t* new_fluid_list(void);
void delete_fluid_list(fluid_list_t *list);
void delete1_fluid_list(fluid_list_t *list);
fluid_list_t* fluid_list_sort(fluid_list_t *list, fluid_compare_func_t compare_func);
fluid_list_t* fluid_list_append(fluid_list_t *list, void* data);
fluid_list_t* fluid_list_prepend(fluid_list_t *list, void* data);
fluid_list_t* fluid_list_remove(fluid_list_t *list, void* data);
fluid_list_t* fluid_list_remove_link(fluid_list_t *list, fluid_list_t *llink);
fluid_list_t* fluid_list_nth(fluid_list_t *list, int n);
fluid_list_t* fluid_list_last(fluid_list_t *list);

#define fluid_list_next(slist)	((slist) ? (((fluid_list_t *)(slist))->next) : NULL)

/* ==== from fluid_list.c: ==== */

fluid_list_t*
new_fluid_list(void)
{
  fluid_list_t* list;
  list = (fluid_list_t*) FLUID_MALLOC(sizeof(fluid_list_t));
  list->data = NULL;
  list->next = NULL;
  return list;
}

void
delete_fluid_list(fluid_list_t *list)
{
  fluid_list_t *next;
  while (list) {
    next = list->next;
    FLUID_FREE(list);
    list = next;
  }
}

void
delete1_fluid_list(fluid_list_t *list)
{
  if (list) {
    FLUID_FREE(list);
  }
}

fluid_list_t*
fluid_list_append(fluid_list_t *list, void*  data)
{
  fluid_list_t *new_list;
  fluid_list_t *last;

  new_list = new_fluid_list();
  new_list->data = data;

  if (list)
    {
      last = fluid_list_last(list);
      /* g_assert (last != NULL); */
      last->next = new_list;

      return list;
    }
  else
      return new_list;
}

fluid_list_t*
fluid_list_prepend(fluid_list_t *list, void* data)
{
  fluid_list_t *new_list;

  new_list = new_fluid_list();
  new_list->data = data;
  new_list->next = list;

  return new_list;
}

fluid_list_t*
fluid_list_nth(fluid_list_t *list, int n)
{
  while ((n-- > 0) && list) {
    list = list->next;
  }

  return list;
}

fluid_list_t*
fluid_list_remove(fluid_list_t *list, void* data)
{
  fluid_list_t *tmp;
  fluid_list_t *prev;

  prev = NULL;
  tmp = list;

  while (tmp) {
    if (tmp->data == data) {
      if (prev) {
	prev->next = tmp->next;
      }
      if (list == tmp) {
	list = list->next;
      }
      tmp->next = NULL;
      delete_fluid_list(tmp);
      
      break;
    }
    
    prev = tmp;
    tmp = tmp->next;
  }

  return list;
}

fluid_list_t*
fluid_list_remove_link(fluid_list_t *list, fluid_list_t *link)
{
  fluid_list_t *tmp;
  fluid_list_t *prev;

  prev = NULL;
  tmp = list;

  while (tmp) {
    if (tmp == link) {
      if (prev) {
	prev->next = tmp->next;
      }
      if (list == tmp) {
	list = list->next;
      }
      tmp->next = NULL;
      break;
    }
    
    prev = tmp;
    tmp = tmp->next;
  }
  
  return list;
}

static fluid_list_t* 
fluid_list_sort_merge(fluid_list_t *l1, fluid_list_t *l2, fluid_compare_func_t compare_func)
{
  fluid_list_t list, *l;

  l = &list;

  while (l1 && l2) {
    if (compare_func(l1->data,l2->data) < 0) {
      l = l->next = l1;
      l1 = l1->next;
    } else {
      l = l->next = l2;
      l2 = l2->next;
    }
  }
  l->next= l1 ? l1 : l2;
  
  return list.next;
}

fluid_list_t* 
fluid_list_sort(fluid_list_t *list, fluid_compare_func_t compare_func)
{
  fluid_list_t *l1, *l2;

  if (!list) {
    return NULL;
  }
  if (!list->next) {
    return list;
  }

  l1 = list; 
  l2 = list->next;

  while ((l2 = l2->next) != NULL) {
    if ((l2 = l2->next) == NULL) 
      break;
    l1=l1->next;
  }
  l2 = l1->next; 
  l1->next = NULL;

  return fluid_list_sort_merge(fluid_list_sort(list, compare_func),
			      fluid_list_sort(l2, compare_func),
			      compare_func);
}


fluid_list_t*
fluid_list_last(fluid_list_t *list)
{
  if (list) {
    while (list->next)
      list = list->next;
  }

  return list;
}

/* ==== from fluid_defsfont.h: ==== */

typedef struct _SFMod
{				/* Modulator structure */
  unsigned short src;			/* source modulator */
  unsigned short dest;			/* destination generator */
  signed short amount;		/* signed, degree of modulation */
  unsigned short amtsrc;		/* second source controls amnt of first */
  unsigned short trans;		/* transform applied to source */
}
SFMod;

typedef union _SFGenAmount
{				/* Generator amount structure */
  signed short sword;			/* signed 16 bit value */
  unsigned short uword;		/* unsigned 16 bit value */
  struct
  {
    unsigned char lo;			/* low value for ranges */
    unsigned char hi;			/* high value for ranges */
  }
  range;
}
SFGenAmount;

typedef struct _SFGen
{				/* Generator structure */
  unsigned short id;			/* generator ID */
  SFGenAmount amount;		/* generator value */
}
SFGen;

typedef struct _SFZone
{				/* Sample/instrument zone structure */
  fluid_list_t *instsamp;		/* instrument/sample pointer for zone */
  fluid_list_t *gen;			/* list of generators */
  fluid_list_t *mod;			/* list of modulators */
}
SFZone;

typedef struct _SFSample
{				/* Sample structure */
  char name[21];		/* Name of sample */
  unsigned char samfile;		/* Loaded sfont/sample buffer = 0/1 */
  unsigned int start;		/* Offset in sample area to start of sample */
  unsigned int end;			/* Offset from start to end of sample,
				   this is the last point of the
				   sample, the SF spec has this as the
				   1st point after, corrected on
				   load/save */
  unsigned int loopstart;		/* Offset from start to start of loop */
  unsigned int loopend;		/* Offset from start to end of loop,
				   marks the first point after loop,
				   whose sample value is ideally
				   equivalent to loopstart */
  unsigned int samplerate;		/* Sample rate recorded at */
  unsigned char origpitch;		/* root midi key number */
  signed char pitchadj;		/* pitch correction in cents */
  unsigned short sampletype;		/* 1 mono,2 right,4 left,linked 8,0x8000=ROM */
}
SFSample;

typedef struct _SFInst
{				/* Instrument structure */
  char name[21];		/* Name of instrument */
  fluid_list_t *zone;			/* list of instrument zones */
}
SFInst;

/* sf file chunk IDs */
enum
{ UNKN_ID, RIFF_ID, LIST_ID, SFBK_ID,
  INFO_ID, SDTA_ID, PDTA_ID,	/* info/sample/preset */

  IFIL_ID, ISNG_ID, INAM_ID, IROM_ID, /* info ids (1st byte of info strings) */
  IVER_ID, ICRD_ID, IENG_ID, IPRD_ID,	/* more info ids */
  ICOP_ID, ICMT_ID, ISFT_ID,	/* and yet more info ids */

  SNAM_ID, SMPL_ID,		/* sample ids */
  PHDR_ID, PBAG_ID, PMOD_ID, PGEN_ID,	/* preset ids */
  IHDR_ID, IBAG_ID, IMOD_ID, IGEN_ID,	/* instrument ids */
  SHDR_ID			/* sample info */
};

/* generator types */
typedef enum
{ Gen_StartAddrOfs, Gen_EndAddrOfs, Gen_StartLoopAddrOfs,
  Gen_EndLoopAddrOfs, Gen_StartAddrCoarseOfs, Gen_ModLFO2Pitch,
  Gen_VibLFO2Pitch, Gen_ModEnv2Pitch, Gen_FilterFc, Gen_FilterQ,
  Gen_ModLFO2FilterFc, Gen_ModEnv2FilterFc, Gen_EndAddrCoarseOfs,
  Gen_ModLFO2Vol, Gen_Unused1, Gen_ChorusSend, Gen_ReverbSend, Gen_Pan,
  Gen_Unused2, Gen_Unused3, Gen_Unused4,
  Gen_ModLFODelay, Gen_ModLFOFreq, Gen_VibLFODelay, Gen_VibLFOFreq,
  Gen_ModEnvDelay, Gen_ModEnvAttack, Gen_ModEnvHold, Gen_ModEnvDecay,
  Gen_ModEnvSustain, Gen_ModEnvRelease, Gen_Key2ModEnvHold,
  Gen_Key2ModEnvDecay, Gen_VolEnvDelay, Gen_VolEnvAttack,
  Gen_VolEnvHold, Gen_VolEnvDecay, Gen_VolEnvSustain, Gen_VolEnvRelease,
  Gen_Key2VolEnvHold, Gen_Key2VolEnvDecay, Gen_Instrument,
  Gen_Reserved1, Gen_KeyRange, Gen_VelRange,
  Gen_StartLoopAddrCoarseOfs, Gen_Keynum, Gen_Velocity,
  Gen_Attenuation, Gen_Reserved2, Gen_EndLoopAddrCoarseOfs,
  Gen_CoarseTune, Gen_FineTune, Gen_SampleId, Gen_SampleModes,
  Gen_Reserved3, Gen_ScaleTune, Gen_ExclusiveClass, Gen_OverrideRootKey,
  Gen_Dummy
}
Gen_Type;

#define Gen_MaxValid 	Gen_Dummy - 1	/* maximum valid generator */

/* global data */

extern unsigned short badgen[]; 	/* list of bad generators */
extern unsigned short badpgen[]; 	/* list of bad preset generators */

/* functions */
void sfont_close (SFData * sf);
void sfont_free_zone (SFZone * zone);
int sfont_preset_compare_func (void* a, void* b);

void sfont_zone_delete (SFData * sf, fluid_list_t ** zlist, SFZone * zone);

fluid_list_t *gen_inlist (int gen, fluid_list_t * genlist);
int gen_valid (int gen);
int gen_validp (int gen);


/*-----------------------------------sffile.h----------------------------*/
/* 
   File structures and routines (used to be in sffile.h) 
*/

#define CHNKIDSTR(id)           &idlist[(id - 1) * 4]

/* sfont file chunk sizes */
#define SFPHDRSIZE	38
#define SFBAGSIZE	4
#define SFMODSIZE	10
#define SFGENSIZE	4
#define SFIHDRSIZE	22
#define SFSHDRSIZE	46

/* sfont file data structures */
typedef struct _SFChunk
{				/* RIFF file chunk structure */
  unsigned int id;			/* chunk id */
  unsigned int size;			/* size of the following chunk */
}
SFChunk;

/* data */
extern char idlist[];

/********************************************************************************/

/* GLIB - Library of useful routines for C programming
 */


/* Provide definitions for some commonly used macros.
 *  Some of them are only provided if they haven't already
 *  been defined. It is assumed that if they are already
 *  defined then the current definition is correct.
 */
#ifndef	FALSE
#define	FALSE	(0)
#endif

#ifndef	TRUE
#define	TRUE	(!FALSE)
#endif

#define GPOINTER_TO_INT(p)	((int)   (p))



/* Provide simple macro statement wrappers (adapted from Perl):
 *  G_STMT_START { statements; } G_STMT_END;
 *  can be used as a single statement, as in
 *  if (x) G_STMT_START { ... } G_STMT_END; else ...
 *
 *  For gcc we will wrap the statements within `({' and `})' braces.
 *  For SunOS they will be wrapped within `if (1)' and `else (void) 0',
 *  and otherwise within `do' and `while (0)'.
 */
#if !(defined (G_STMT_START) && defined (G_STMT_END))
#  if defined (__GNUC__) && !defined (__STRICT_ANSI__) && !defined (__cplusplus)
#    define G_STMT_START	(void)(
#    define G_STMT_END		)
#  else
#    if (defined (sun) || defined (__sun__))
#      define G_STMT_START	if (1)
#      define G_STMT_END	else (void)0
#    else
#      define G_STMT_START	do
#      define G_STMT_END	while (0)
#    endif
#  endif
#endif


/* Basic bit swapping functions
 */
#define GUINT16_SWAP_LE_BE_CONSTANT(val)	((unsigned short) ( \
    (((unsigned short) (val) & (unsigned short) 0x00ffU) << 8) | \
    (((unsigned short) (val) & (unsigned short) 0xff00U) >> 8)))
#define GUINT32_SWAP_LE_BE_CONSTANT(val)	((unsigned int) ( \
    (((unsigned int) (val) & (unsigned int) 0x000000ffU) << 24) | \
    (((unsigned int) (val) & (unsigned int) 0x0000ff00U) <<  8) | \
    (((unsigned int) (val) & (unsigned int) 0x00ff0000U) >>  8) | \
    (((unsigned int) (val) & (unsigned int) 0xff000000U) >> 24)))

#define GUINT16_SWAP_LE_BE(val) (GUINT16_SWAP_LE_BE_CONSTANT (val))
#define GUINT32_SWAP_LE_BE(val) (GUINT32_SWAP_LE_BE_CONSTANT (val))

#define GINT16_TO_LE(val)	((signed short) (val))
#define GUINT16_TO_LE(val)	((unsigned short) (val))
#define GINT16_TO_BE(val)	((signed short) GUINT16_SWAP_LE_BE (val))
#define GUINT16_TO_BE(val)	(GUINT16_SWAP_LE_BE (val))
#define GINT32_TO_LE(val)	((signed int) (val))
#define GUINT32_TO_LE(val)	((unsigned int) (val))
#define GINT32_TO_BE(val)	((signed int) GUINT32_SWAP_LE_BE (val))
#define GUINT32_TO_BE(val)	(GUINT32_SWAP_LE_BE (val))

/* The G*_TO_?E() macros are defined in glibconfig.h.
 * The transformation is symmetric, so the FROM just maps to the TO.
 */
#define GINT16_FROM_LE(val)	(GINT16_TO_LE (val))
#define GUINT16_FROM_LE(val)	(GUINT16_TO_LE (val))
#define GINT16_FROM_BE(val)	(GINT16_TO_BE (val))
#define GUINT16_FROM_BE(val)	(GUINT16_TO_BE (val))
#define GINT32_FROM_LE(val)	(GINT32_TO_LE (val))
#define GUINT32_FROM_LE(val)	(GUINT32_TO_LE (val))
#define GINT32_FROM_BE(val)	(GINT32_TO_BE (val))
#define GUINT32_FROM_BE(val)	(GUINT32_TO_BE (val))


/*-----------------------------------util.h----------------------------*/
/*
  Utility functions (formerly in util.h)
 */
#define FAIL	0
#define OK	1

enum
{ ErrWarn, ErrFatal, ErrStatus, ErrCorr, ErrEof, ErrMem, Errno,
  ErrRead, ErrWrite
};

int gerr (int ev, char * fmt, ...);
void *safe_malloc (size_t size);
int safe_fread (void *buf, int count, FILE * fd);
int safe_fseek (FILE * fd, long ofs, int whence);

/* ==== from fluid_defsfont.c: ==== */

/*=================================sfload.c========================
  Borrowed from Smurf SoundFont Editor by Josh Green
  =================================================================*/

/*
   functions for loading data from sfont files, with appropriate byte swapping
   on big endian machines. Sfont IDs are not swapped because the ID read is
   equivalent to the matching ID list in memory regardless of LE/BE machine
*/

#ifdef WORDS_BIGENDIAN
#define READCHUNK(var,fd)	G_STMT_START {		\
	if (!safe_fread(var, 8, fd))			\
	return(FAIL);					\
	((SFChunk *)(var))->size = GUINT32_FROM_BE(((SFChunk *)(var))->size);  \
} G_STMT_END
#else
#define READCHUNK(var,fd)	G_STMT_START {		\
    if (!safe_fread(var, 8, fd))			\
	return(FAIL);					\
    ((SFChunk *)(var))->size = GUINT32_FROM_LE(((SFChunk *)(var))->size);  \
} G_STMT_END
#endif
#define READID(var,fd)		G_STMT_START {		\
    if (!safe_fread(var, 4, fd))			\
	return(FAIL);					\
} G_STMT_END
#define READSTR(var,fd)		G_STMT_START {		\
    if (!safe_fread(var, 20, fd))			\
	return(FAIL);					\
    (*var)[20] = '\0';					\
} G_STMT_END
#ifdef WORDS_BIGENDIAN
#define READD(var,fd)		G_STMT_START {		\
	unsigned int _temp;					\
	if (!safe_fread(&_temp, 4, fd))			\
	return(FAIL);					\
	var = GINT32_FROM_BE(_temp);			\
} G_STMT_END
#else
#define READD(var,fd)		G_STMT_START {		\
    unsigned int _temp;					\
    if (!safe_fread(&_temp, 4, fd))			\
	return(FAIL);					\
    var = GINT32_FROM_LE(_temp);			\
} G_STMT_END
#endif
#ifdef WORDS_BIGENDIAN
#define READW(var,fd)		G_STMT_START {		\
	unsigned short _temp;					\
	if (!safe_fread(&_temp, 2, fd))			\
	return(FAIL);					\
var = GINT16_FROM_BE(_temp);			\
} G_STMT_END
#else
#define READW(var,fd)		G_STMT_START {		\
    unsigned short _temp;					\
    if (!safe_fread(&_temp, 2, fd))			\
	return(FAIL);					\
    var = GINT16_FROM_LE(_temp);			\
} G_STMT_END
#endif
#define READB(var,fd)		G_STMT_START {		\
    if (!safe_fread(&var, 1, fd))			\
	return(FAIL);					\
} G_STMT_END
#define FSKIP(size,fd)		G_STMT_START {		\
    if (!safe_fseek(fd, size, SEEK_CUR))		\
	return(FAIL);					\
} G_STMT_END
#define FSKIPW(fd)		G_STMT_START {		\
    if (!safe_fseek(fd, 2, SEEK_CUR))			\
	return(FAIL);					\
} G_STMT_END

/* removes and advances a fluid_list_t pointer */
#define SLADVREM(list, item)	G_STMT_START {		\
    fluid_list_t *_temp = item;				\
    item = fluid_list_next(item);				\
    list = fluid_list_remove_link(list, _temp);		\
    delete1_fluid_list(_temp);				\
} G_STMT_END

static int chunkid (unsigned int id);
static int load_body (unsigned int size, SFData * sf, FILE * fd);
static int read_listchunk (SFChunk * chunk, FILE * fd);
static int process_info (int size, SFData * sf, FILE * fd);
static int process_sdta (int size, SFData * sf, FILE * fd);
static int pdtahelper (unsigned int expid, unsigned int reclen, SFChunk * chunk,
  int * size, FILE * fd);
static int process_pdta (int size, SFData * sf, FILE * fd);
static int load_phdr (int size, SFData * sf, FILE * fd);
static int load_pbag (int size, SFData * sf, FILE * fd);
static int load_pmod (int size, SFData * sf, FILE * fd);
static int load_pgen (int size, SFData * sf, FILE * fd);
static int load_ihdr (int size, SFData * sf, FILE * fd);
static int load_ibag (int size, SFData * sf, FILE * fd);
static int load_imod (int size, SFData * sf, FILE * fd);
static int load_igen (int size, SFData * sf, FILE * fd);
static int load_shdr (unsigned int size, SFData * sf, FILE * fd);
static int fixup_pgen (SFData * sf);
static int fixup_igen (SFData * sf);
static int fixup_sample (SFData * sf);

char idlist[] = {
  "RIFFLISTsfbkINFOsdtapdtaifilisngINAMiromiverICRDIENGIPRD"
    "ICOPICMTISFTsnamsmplphdrpbagpmodpgeninstibagimodigenshdr"
};

static unsigned int sdtachunk_size;

/* sound font file load functions */
static int
chunkid (unsigned int id)
{
  unsigned int i;
  unsigned int *p;

  p = (unsigned int *) & idlist;
  for (i = 0; i < sizeof (idlist) / sizeof (int); i++, p += 1)
    if (*p == id)
      return (i + 1);

  return (UNKN_ID);
}

SFData *
fsd_sfload_file (const char * fname)
{
  SFData *sf = NULL;
  FILE *fd;
  int fsize = 0;
  int err = FALSE;

  if (!(fd = fopen (fname, "rb")))
    {
      FLUID_LOG (FLUID_ERR, _("Unable to open file \"%s\""), fname);
      return (NULL);
    }

  if (!(sf = safe_malloc (sizeof (SFData))))
    err = TRUE;
  if (!err)
    {
      memset (sf, 0, sizeof (SFData));	/* zero sfdata */
      sf->fname = FLUID_STRDUP (fname);	/* copy file name */
      sf->sffd = fd;
    }

  /* get size of file */
  if (!err && fseek (fd, 0L, SEEK_END) == -1)
    {				/* seek to end of file */
      err = TRUE;
      FLUID_LOG (FLUID_ERR, _("Seek to end of file failed"));
    }
  if (!err && (fsize = ftell (fd)) == -1)
    {				/* position = size */
      err = TRUE;
      FLUID_LOG (FLUID_ERR, _("Get end of file position failed"));
    }
  if (!err)
    rewind (fd);

  if (!err && !load_body (fsize, sf, fd))
    err = TRUE;			/* load the sfont */

    /* ==== fsd addition: since we're not loaded the sample data, we don't need
     * to keep the file open. */
    fclose(fd);
    sf->sffd = NULL;
    /* ==== end fsd addition. */

  if (err)
    {
      if (sf)
	sfont_close (sf);
      return (NULL);
    }

  return (sf);
}

static int
load_body (unsigned int size, SFData * sf, FILE * fd)
{
  SFChunk chunk;

  READCHUNK (&chunk, fd);	/* load RIFF chunk */
  if (chunkid (chunk.id) != RIFF_ID) {	/* error if not RIFF */
    FLUID_LOG (FLUID_ERR, _("Not a RIFF file"));
    return (FAIL);
  }

  READID (&chunk.id, fd);	/* load file ID */
  if (chunkid (chunk.id) != SFBK_ID) {	/* error if not SFBK_ID */
    FLUID_LOG (FLUID_ERR, _("Not a sound font file"));
    return (FAIL);
  }

  if (chunk.size != size - 8) {
    gerr (ErrCorr, _("Sound font file size mismatch"));
    return (FAIL);
  }

  /* Process INFO block */
  if (!read_listchunk (&chunk, fd))
    return (FAIL);
  if (chunkid (chunk.id) != INFO_ID)
    return (gerr (ErrCorr, _("Invalid ID found when expecting INFO chunk")));
  if (!process_info (chunk.size, sf, fd))
    return (FAIL);

  /* Process sample chunk */
  if (!read_listchunk (&chunk, fd))
    return (FAIL);
  if (chunkid (chunk.id) != SDTA_ID)
    return (gerr (ErrCorr,
	_("Invalid ID found when expecting SAMPLE chunk")));
  if (!process_sdta (chunk.size, sf, fd))
    return (FAIL);

  /* process HYDRA chunk */
  if (!read_listchunk (&chunk, fd))
    return (FAIL);
  if (chunkid (chunk.id) != PDTA_ID)
    return (gerr (ErrCorr, _("Invalid ID found when expecting HYDRA chunk")));
  if (!process_pdta (chunk.size, sf, fd))
    return (FAIL);

  if (!fixup_pgen (sf))
    return (FAIL);
  if (!fixup_igen (sf))
    return (FAIL);
  if (!fixup_sample (sf))
    return (FAIL);

  /* sort preset list by bank, preset # */
  sf->preset = fluid_list_sort (sf->preset,
    (fluid_compare_func_t) sfont_preset_compare_func);

  return (OK);
}

static int
read_listchunk (SFChunk * chunk, FILE * fd)
{
  READCHUNK (chunk, fd);	/* read list chunk */
  if (chunkid (chunk->id) != LIST_ID)	/* error if ! list chunk */
    return (gerr (ErrCorr, _("Invalid chunk id in level 0 parse")));
  READID (&chunk->id, fd);	/* read id string */
  chunk->size -= 4;
  return (OK);
}

static int
process_info (int size, SFData * sf, FILE * fd)
{
  SFChunk chunk;
  unsigned char id;
  char *item;
  unsigned short ver;

  while (size > 0)
    {
      READCHUNK (&chunk, fd);
      size -= 8;

      id = chunkid (chunk.id);

      if (id == IFIL_ID)
	{			/* sound font version chunk? */
	  if (chunk.size != 4)
	    return (gerr (ErrCorr,
		_("Sound font version info chunk has invalid size")));

	  READW (ver, fd);
	  sf->version.major = ver;
	  READW (ver, fd);
	  sf->version.minor = ver;

	  if (sf->version.major < 2) {
	    FLUID_LOG (FLUID_ERR,
		      _("Sound font version is %d.%d which is not"
			" supported, convert to version 2.0x"), 
		      sf->version.major,
		      sf->version.minor);
	    return (FAIL);
	  }

	  if (sf->version.major > 2) {
	    FLUID_LOG (FLUID_WARN, 
		      _("Sound font version is %d.%d which is newer than"
			" what this version of FLUID Synth was designed for (v2.0x)"), 
		      sf->version.major,
		      sf->version.minor);
	    return (FAIL);
	  }
	}
      else if (id == IVER_ID)
	{			/* ROM version chunk? */
	  if (chunk.size != 4)
	    return (gerr (ErrCorr,
		_("ROM version info chunk has invalid size")));

	  READW (ver, fd);
	  sf->romver.major = ver;
	  READW (ver, fd);
	  sf->romver.minor = ver;
	}
      else if (id != UNKN_ID)
	{
	  if ((id != ICMT_ID && chunk.size > 256) || (chunk.size > 65536)
	    || (chunk.size % 2))
	    return (gerr (ErrCorr,
		_("INFO sub chunk %.4s has invalid chunk size"
		  " of %d bytes"), &chunk.id, chunk.size));

	  /* alloc for chunk id and da chunk */
	  if (!(item = safe_malloc (chunk.size + 1)))
	    return (FAIL);

	  /* attach to INFO list, sfont_close will cleanup if FAIL occurs */
	  sf->info = fluid_list_append (sf->info, item);

	  *(unsigned char *) item = id;
	  if (!safe_fread (&item[1], chunk.size, fd))
	    return (FAIL);

	  /* force terminate info item (don't forget uint8 info ID) */
	  *(item + chunk.size) = '\0';
	}
      else
	return (gerr (ErrCorr, _("Invalid chunk id in INFO chunk")));
      size -= chunk.size;
    }

  if (size < 0)
    return (gerr (ErrCorr, _("INFO chunk size mismatch")));

  return (OK);
}

static int
process_sdta (int size, SFData * sf, FILE * fd)
{
  SFChunk chunk;

  if (size == 0)
    return (OK);		/* no sample data? */

  /* read sub chunk */
  READCHUNK (&chunk, fd);
  size -= 8;

  if (chunkid (chunk.id) != SMPL_ID)
    return (gerr (ErrCorr,
	_("Expected SMPL chunk found invalid id instead")));

  if ((size - chunk.size) != 0)
    return (gerr (ErrCorr, _("SDTA chunk size mismatch")));

  /* sample data follows */
  sf->samplepos = ftell (fd);

  /* used in fixup_sample() to check validity of sample headers */
  sdtachunk_size = chunk.size;
  sf->samplesize = chunk.size;

  FSKIP (chunk.size, fd);

  return (OK);
}

static int
pdtahelper (unsigned int expid, unsigned int reclen, SFChunk * chunk,
  int * size, FILE * fd)
{
  unsigned int id;
  char *expstr;

  expstr = CHNKIDSTR (expid);	/* in case we need it */

  READCHUNK (chunk, fd);
  *size -= 8;

  if ((id = chunkid (chunk->id)) != expid)
    return (gerr (ErrCorr, _("Expected"
	  " PDTA sub-chunk \"%.4s\" found invalid id instead"), expstr));

  if (chunk->size % reclen)	/* valid chunk size? */
    return (gerr (ErrCorr,
	_("\"%.4s\" chunk size is not a multiple of %d bytes"), expstr,
	reclen));
  if ((*size -= chunk->size) < 0)
    return (gerr (ErrCorr,
	_("\"%.4s\" chunk size exceeds remaining PDTA chunk size"), expstr));
  return (OK);
}

static int
process_pdta (int size, SFData * sf, FILE * fd)
{
  SFChunk chunk;

  if (!pdtahelper (PHDR_ID, SFPHDRSIZE, &chunk, &size, fd))
    return (FAIL);
  if (!load_phdr (chunk.size, sf, fd))
    return (FAIL);

  if (!pdtahelper (PBAG_ID, SFBAGSIZE, &chunk, &size, fd))
    return (FAIL);
  if (!load_pbag (chunk.size, sf, fd))
    return (FAIL);

  if (!pdtahelper (PMOD_ID, SFMODSIZE, &chunk, &size, fd))
    return (FAIL);
  if (!load_pmod (chunk.size, sf, fd))
    return (FAIL);

  if (!pdtahelper (PGEN_ID, SFGENSIZE, &chunk, &size, fd))
    return (FAIL);
  if (!load_pgen (chunk.size, sf, fd))
    return (FAIL);

  if (!pdtahelper (IHDR_ID, SFIHDRSIZE, &chunk, &size, fd))
    return (FAIL);
  if (!load_ihdr (chunk.size, sf, fd))
    return (FAIL);

  if (!pdtahelper (IBAG_ID, SFBAGSIZE, &chunk, &size, fd))
    return (FAIL);
  if (!load_ibag (chunk.size, sf, fd))
    return (FAIL);

  if (!pdtahelper (IMOD_ID, SFMODSIZE, &chunk, &size, fd))
    return (FAIL);
  if (!load_imod (chunk.size, sf, fd))
    return (FAIL);

  if (!pdtahelper (IGEN_ID, SFGENSIZE, &chunk, &size, fd))
    return (FAIL);
  if (!load_igen (chunk.size, sf, fd))
    return (FAIL);

  if (!pdtahelper (SHDR_ID, SFSHDRSIZE, &chunk, &size, fd))
    return (FAIL);
  if (!load_shdr (chunk.size, sf, fd))
    return (FAIL);

  return (OK);
}

/* preset header loader */
static int
load_phdr (int size, SFData * sf, FILE * fd)
{
  int i, i2;
  SFPreset *p, *pr = NULL;	/* ptr to current & previous preset */
  unsigned short zndx, pzndx = 0;

  if (size % SFPHDRSIZE || size == 0)
    return (gerr (ErrCorr, _("Preset header chunk size is invalid")));

  i = size / SFPHDRSIZE - 1;
  if (i == 0)
    {				/* at least one preset + term record */
      FLUID_LOG (FLUID_WARN, _("File contains no presets"));
      FSKIP (SFPHDRSIZE, fd);
      return (OK);
    }

  for (; i > 0; i--)
    {				/* load all preset headers */
      p = FLUID_NEW (SFPreset);
      sf->preset = fluid_list_append (sf->preset, p);
      p->zone = NULL;		/* In case of failure, sfont_close can cleanup */
      READSTR (&p->name, fd);	/* possible read failure ^ */
      READW (p->prenum, fd);
      READW (p->bank, fd);
      READW (zndx, fd);
      READD (p->libr, fd);
      READD (p->genre, fd);
      READD (p->morph, fd);

      if (pr)
	{			/* not first preset? */
	  if (zndx < pzndx)
	    return (gerr (ErrCorr, _("Preset header indices not monotonic")));
	  i2 = zndx - pzndx;
	  while (i2--)
	    {
	      pr->zone = fluid_list_prepend (pr->zone, NULL);
	    }
	}
      else if (zndx > 0)	/* 1st preset, warn if ofs >0 */
	FLUID_LOG (FLUID_WARN, _("%d preset zones not referenced, discarding"), zndx);
      pr = p;			/* update preset ptr */
      pzndx = zndx;
    }

  FSKIP (24, fd);
  READW (zndx, fd);		/* Read terminal generator index */
  FSKIP (12, fd);

  if (zndx < pzndx)
    return (gerr (ErrCorr, _("Preset header indices not monotonic")));
  i2 = zndx - pzndx;
  while (i2--)
    {
      pr->zone = fluid_list_prepend (pr->zone, NULL);
    }

  return (OK);
}

/* preset bag loader */
static int
load_pbag (int size, SFData * sf, FILE * fd)
{
  fluid_list_t *p, *p2;
  SFZone *z, *pz = NULL;
  unsigned short genndx, modndx;
  unsigned short pgenndx = 0, pmodndx = 0;
  unsigned short i;

  if (size % SFBAGSIZE || size == 0)	/* size is multiple of SFBAGSIZE? */
    return (gerr (ErrCorr, _("Preset bag chunk size is invalid")));

  p = sf->preset;
  while (p)
    {				/* traverse through presets */
      p2 = ((SFPreset *) (p->data))->zone;
      while (p2)
	{			/* traverse preset's zones */
	  if ((size -= SFBAGSIZE) < 0)
	    return (gerr (ErrCorr, _("Preset bag chunk size mismatch")));
	  z = FLUID_NEW (SFZone);
	  p2->data = z;
	  z->gen = NULL;	/* Init gen and mod before possible failure, */
	  z->mod = NULL;	/* to ensure proper cleanup (sfont_close) */
	  READW (genndx, fd);	/* possible read failure ^ */
	  READW (modndx, fd);
	  z->instsamp = NULL;

	  if (pz)
	    {			/* if not first zone */
	      if (genndx < pgenndx)
		return (gerr (ErrCorr,
		    _("Preset bag generator indices not monotonic")));
	      if (modndx < pmodndx)
		return (gerr (ErrCorr,
		    _("Preset bag modulator indices not monotonic")));
	      i = genndx - pgenndx;
	      while (i--)
		pz->gen = fluid_list_prepend (pz->gen, NULL);
	      i = modndx - pmodndx;
	      while (i--)
		pz->mod = fluid_list_prepend (pz->mod, NULL);
	    }
	  pz = z;		/* update previous zone ptr */
	  pgenndx = genndx;	/* update previous zone gen index */
	  pmodndx = modndx;	/* update previous zone mod index */
	  p2 = fluid_list_next (p2);
	}
      p = fluid_list_next (p);
    }

  size -= SFBAGSIZE;
  if (size != 0)
    return (gerr (ErrCorr, _("Preset bag chunk size mismatch")));

  READW (genndx, fd);
  READW (modndx, fd);

  if (!pz)
    {
      if (genndx > 0)
	FLUID_LOG (FLUID_WARN, _("No preset generators and terminal index not 0"));
      if (modndx > 0)
	FLUID_LOG (FLUID_WARN, _("No preset modulators and terminal index not 0"));
      return (OK);
    }

  if (genndx < pgenndx)
    return (gerr (ErrCorr, _("Preset bag generator indices not monotonic")));
  if (modndx < pmodndx)
    return (gerr (ErrCorr, _("Preset bag modulator indices not monotonic")));
  i = genndx - pgenndx;
  while (i--)
    pz->gen = fluid_list_prepend (pz->gen, NULL);
  i = modndx - pmodndx;
  while (i--)
    pz->mod = fluid_list_prepend (pz->mod, NULL);

  return (OK);
}

/* preset modulator loader */
static int
load_pmod (int size, SFData * sf, FILE * fd)
{
  fluid_list_t *p, *p2, *p3;
  SFMod *m;

  p = sf->preset;
  while (p)
    {				/* traverse through all presets */
      p2 = ((SFPreset *) (p->data))->zone;
      while (p2)
	{			/* traverse this preset's zones */
	  p3 = ((SFZone *) (p2->data))->mod;
	  while (p3)
	    {			/* load zone's modulators */
	      if ((size -= SFMODSIZE) < 0)
		return (gerr (ErrCorr,
		    _("Preset modulator chunk size mismatch")));
	      m = FLUID_NEW (SFMod);
	      p3->data = m;
	      READW (m->src, fd);
	      READW (m->dest, fd);
	      READW (m->amount, fd);
	      READW (m->amtsrc, fd);
	      READW (m->trans, fd);
	      p3 = fluid_list_next (p3);
	    }
	  p2 = fluid_list_next (p2);
	}
      p = fluid_list_next (p);
    }

  /*
     If there isn't even a terminal record
     Hmmm, the specs say there should be one, but..
   */
  if (size == 0)
    return (OK);

  size -= SFMODSIZE;
  if (size != 0)
    return (gerr (ErrCorr, _("Preset modulator chunk size mismatch")));
  FSKIP (SFMODSIZE, fd);	/* terminal mod */

  return (OK);
}

/* -------------------------------------------------------------------
 * preset generator loader
 * generator (per preset) loading rules:
 * Zones with no generators or modulators shall be annihilated
 * Global zone must be 1st zone, discard additional ones (instrumentless zones)
 *
 * generator (per zone) loading rules (in order of decreasing precedence):
 * KeyRange is 1st in list (if exists), else discard
 * if a VelRange exists only preceded by a KeyRange, else discard
 * if a generator follows an instrument discard it
 * if a duplicate generator exists replace previous one
 * ------------------------------------------------------------------- */
static int
load_pgen (int size, SFData * sf, FILE * fd)
{
  fluid_list_t *p, *p2, *p3, *dup, **hz = NULL;
  SFZone *z;
  SFGen *g;
  SFGenAmount genval;
  unsigned short genid;
  int level, skip, drop, gzone, discarded;

  p = sf->preset;
  while (p)
    {				/* traverse through all presets */
      gzone = FALSE;
      discarded = FALSE;
      p2 = ((SFPreset *) (p->data))->zone;
      if (p2)
	hz = &p2;
      while (p2)
	{			/* traverse preset's zones */
	  level = 0;
	  z = (SFZone *) (p2->data);
	  p3 = z->gen;
	  while (p3)
	    {			/* load zone's generators */
	      dup = NULL;
	      skip = FALSE;
	      drop = FALSE;
	      if ((size -= SFGENSIZE) < 0)
		return (gerr (ErrCorr,
		    _("Preset generator chunk size mismatch")));

	      READW (genid, fd);

	      if (genid == Gen_KeyRange)
		{		/* nothing precedes */
		  if (level == 0)
		    {
		      level = 1;
		      READB (genval.range.lo, fd);
		      READB (genval.range.hi, fd);
		    }
		  else
		    skip = TRUE;
		}
	      else if (genid == Gen_VelRange)
		{		/* only KeyRange precedes */
		  if (level <= 1)
		    {
		      level = 2;
		      READB (genval.range.lo, fd);
		      READB (genval.range.hi, fd);
		    }
		  else
		    skip = TRUE;
		}
	      else if (genid == Gen_Instrument)
		{		/* inst is last gen */
		  level = 3;
		  READW (genval.uword, fd);
		  ((SFZone *) (p2->data))->instsamp = (fluid_list_t *) (genval.uword + 1);
		  break;	/* break out of generator loop */
		}
	      else
		{
		  level = 2;
		  if (gen_validp (genid))
		    {		/* generator valid? */
		      READW (genval.sword, fd);
		      dup = gen_inlist (genid, z->gen);
		    }
		  else
		    skip = TRUE;
		}

	      if (!skip)
		{
		  if (!dup)
		    {		/* if gen ! dup alloc new */
		      g = FLUID_NEW (SFGen);
		      p3->data = g;
		      g->id = genid;
		    }
		  else
		    {
		      g = (SFGen *) (dup->data);	/* ptr to orig gen */
		      drop = TRUE;
		    }
		  g->amount = genval;
		}
	      else
		{		/* Skip this generator */
		  discarded = TRUE;
		  drop = TRUE;
		  FSKIPW (fd);
		}

	      if (!drop)
		p3 = fluid_list_next (p3);	/* next gen */
	      else
		SLADVREM (z->gen, p3);	/* drop place holder */

	    }			/* generator loop */

	  if (level == 3)
	    SLADVREM (z->gen, p3);	/* zone has inst? */
	  else
	    {			/* congratulations its a global zone */
	      if (!gzone)
		{		/* Prior global zones? */
		  gzone = TRUE;

		  /* if global zone is not 1st zone, relocate */
		  if (*hz != p2)
		    {
		      void* save = p2->data;
		      FLUID_LOG (FLUID_WARN,
			_("Preset \"%s\": Global zone is not first zone"),
			((SFPreset *) (p->data))->name);
		      SLADVREM (*hz, p2);
		      *hz = fluid_list_prepend (*hz, save);
		      continue;
		    }
		}
	      else
		{		/* previous global zone exists, discard */
		  FLUID_LOG (FLUID_WARN,
		    _("Preset \"%s\": Discarding invalid global zone"),
		    ((SFPreset *) (p->data))->name);
		  sfont_zone_delete (sf, hz, (SFZone *) (p2->data));
		}
	    }

	  while (p3)
	    {			/* Kill any zones following an instrument */
	      discarded = TRUE;
	      if ((size -= SFGENSIZE) < 0)
		return (gerr (ErrCorr,
		    _("Preset generator chunk size mismatch")));
	      FSKIP (SFGENSIZE, fd);
	      SLADVREM (z->gen, p3);
	    }

	  p2 = fluid_list_next (p2);	/* next zone */
	}
      if (discarded)
	FLUID_LOG(FLUID_WARN,
	  _("Preset \"%s\": Some invalid generators were discarded"),
	  ((SFPreset *) (p->data))->name);
      p = fluid_list_next (p);
    }

  /* in case there isn't a terminal record */
  if (size == 0)
    return (OK);

  size -= SFGENSIZE;
  if (size != 0)
    return (gerr (ErrCorr, _("Preset generator chunk size mismatch")));
  FSKIP (SFGENSIZE, fd);	/* terminal gen */

  return (OK);
}

/* instrument header loader */
static int
load_ihdr (int size, SFData * sf, FILE * fd)
{
  int i, i2;
  SFInst *p, *pr = NULL;	/* ptr to current & previous instrument */
  unsigned short zndx, pzndx = 0;

  if (size % SFIHDRSIZE || size == 0)	/* chunk size is valid? */
    return (gerr (ErrCorr, _("Instrument header has invalid size")));

  size = size / SFIHDRSIZE - 1;
  if (size == 0)
    {				/* at least one preset + term record */
      FLUID_LOG (FLUID_WARN, _("File contains no instruments"));
      FSKIP (SFIHDRSIZE, fd);
      return (OK);
    }

  for (i = 0; i < size; i++)
    {				/* load all instrument headers */
      p = FLUID_NEW (SFInst);
      sf->inst = fluid_list_append (sf->inst, p);
      p->zone = NULL;		/* For proper cleanup if fail (sfont_close) */
      READSTR (&p->name, fd);	/* Possible read failure ^ */
      READW (zndx, fd);

      if (pr)
	{			/* not first instrument? */
	  if (zndx < pzndx)
	    return (gerr (ErrCorr,
		_("Instrument header indices not monotonic")));
	  i2 = zndx - pzndx;
	  while (i2--)
	    pr->zone = fluid_list_prepend (pr->zone, NULL);
	}
      else if (zndx > 0)	/* 1st inst, warn if ofs >0 */
	FLUID_LOG (FLUID_WARN, _("%d instrument zones not referenced, discarding"),
	  zndx);
      pzndx = zndx;
      pr = p;			/* update instrument ptr */
    }

  FSKIP (20, fd);
  READW (zndx, fd);

  if (zndx < pzndx)
    return (gerr (ErrCorr, _("Instrument header indices not monotonic")));
  i2 = zndx - pzndx;
  while (i2--)
    pr->zone = fluid_list_prepend (pr->zone, NULL);

  return (OK);
}

/* instrument bag loader */
static int
load_ibag (int size, SFData * sf, FILE * fd)
{
  fluid_list_t *p, *p2;
  SFZone *z, *pz = NULL;
  unsigned short genndx, modndx, pgenndx = 0, pmodndx = 0;
  int i;

  if (size % SFBAGSIZE || size == 0)	/* size is multiple of SFBAGSIZE? */
    return (gerr (ErrCorr, _("Instrument bag chunk size is invalid")));

  p = sf->inst;
  while (p)
    {				/* traverse through inst */
      p2 = ((SFInst *) (p->data))->zone;
      while (p2)
	{			/* load this inst's zones */
	  if ((size -= SFBAGSIZE) < 0)
	    return (gerr (ErrCorr, _("Instrument bag chunk size mismatch")));
	  z = FLUID_NEW (SFZone);
	  p2->data = z;
	  z->gen = NULL;	/* In case of failure, */
	  z->mod = NULL;	/* sfont_close can clean up */
	  READW (genndx, fd);	/* READW = possible read failure */
	  READW (modndx, fd);
	  z->instsamp = NULL;

	  if (pz)
	    {			/* if not first zone */
	      if (genndx < pgenndx)
		return (gerr (ErrCorr,
		    _("Instrument generator indices not monotonic")));
	      if (modndx < pmodndx)
		return (gerr (ErrCorr,
		    _("Instrument modulator indices not monotonic")));
	      i = genndx - pgenndx;
	      while (i--)
		pz->gen = fluid_list_prepend (pz->gen, NULL);
	      i = modndx - pmodndx;
	      while (i--)
		pz->mod = fluid_list_prepend (pz->mod, NULL);
	    }
	  pz = z;		/* update previous zone ptr */
	  pgenndx = genndx;
	  pmodndx = modndx;
	  p2 = fluid_list_next (p2);
	}
      p = fluid_list_next (p);
    }

  size -= SFBAGSIZE;
  if (size != 0)
    return (gerr (ErrCorr, _("Instrument chunk size mismatch")));

  READW (genndx, fd);
  READW (modndx, fd);

  if (!pz)
    {				/* in case that all are no zoners */
      if (genndx > 0)
	FLUID_LOG (FLUID_WARN,
	  _("No instrument generators and terminal index not 0"));
      if (modndx > 0)
	FLUID_LOG (FLUID_WARN,
	  _("No instrument modulators and terminal index not 0"));
      return (OK);
    }

  if (genndx < pgenndx)
    return (gerr (ErrCorr, _("Instrument generator indices not monotonic")));
  if (modndx < pmodndx)
    return (gerr (ErrCorr, _("Instrument modulator indices not monotonic")));
  i = genndx - pgenndx;
  while (i--)
    pz->gen = fluid_list_prepend (pz->gen, NULL);
  i = modndx - pmodndx;
  while (i--)
    pz->mod = fluid_list_prepend (pz->mod, NULL);

  return (OK);
}

/* instrument modulator loader */
static int
load_imod (int size, SFData * sf, FILE * fd)
{
  fluid_list_t *p, *p2, *p3;
  SFMod *m;

  p = sf->inst;
  while (p)
    {				/* traverse through all inst */
      p2 = ((SFInst *) (p->data))->zone;
      while (p2)
	{			/* traverse this inst's zones */
	  p3 = ((SFZone *) (p2->data))->mod;
	  while (p3)
	    {			/* load zone's modulators */
	      if ((size -= SFMODSIZE) < 0)
		return (gerr (ErrCorr,
		    _("Instrument modulator chunk size mismatch")));
	      m = FLUID_NEW (SFMod);
	      p3->data = m;
	      READW (m->src, fd);
	      READW (m->dest, fd);
	      READW (m->amount, fd);
	      READW (m->amtsrc, fd);
	      READW (m->trans, fd);
	      p3 = fluid_list_next (p3);
	    }
	  p2 = fluid_list_next (p2);
	}
      p = fluid_list_next (p);
    }

  /*
     If there isn't even a terminal record
     Hmmm, the specs say there should be one, but..
   */
  if (size == 0)
    return (OK);

  size -= SFMODSIZE;
  if (size != 0)
    return (gerr (ErrCorr, _("Instrument modulator chunk size mismatch")));
  FSKIP (SFMODSIZE, fd);	/* terminal mod */

  return (OK);
}

/* load instrument generators (see load_pgen for loading rules) */
static int
load_igen (int size, SFData * sf, FILE * fd)
{
  fluid_list_t *p, *p2, *p3, *dup, **hz = NULL;
  SFZone *z;
  SFGen *g;
  SFGenAmount genval;
  unsigned short genid;
  int level, skip, drop, gzone, discarded;

  p = sf->inst;
  while (p)
    {				/* traverse through all instruments */
      gzone = FALSE;
      discarded = FALSE;
      p2 = ((SFInst *) (p->data))->zone;
      if (p2)
	hz = &p2;
      while (p2)
	{			/* traverse this instrument's zones */
	  level = 0;
	  z = (SFZone *) (p2->data);
	  p3 = z->gen;
	  while (p3)
	    {			/* load zone's generators */
	      dup = NULL;
	      skip = FALSE;
	      drop = FALSE;
	      if ((size -= SFGENSIZE) < 0)
		return (gerr (ErrCorr, _("IGEN chunk size mismatch")));

	      READW (genid, fd);

	      if (genid == Gen_KeyRange)
		{		/* nothing precedes */
		  if (level == 0)
		    {
		      level = 1;
		      READB (genval.range.lo, fd);
		      READB (genval.range.hi, fd);
		    }
		  else
		    skip = TRUE;
		}
	      else if (genid == Gen_VelRange)
		{		/* only KeyRange precedes */
		  if (level <= 1)
		    {
		      level = 2;
		      READB (genval.range.lo, fd);
		      READB (genval.range.hi, fd);
		    }
		  else
		    skip = TRUE;
		}
	      else if (genid == Gen_SampleId)
		{		/* sample is last gen */
		  level = 3;
		  READW (genval.uword, fd);
		  ((SFZone *) (p2->data))->instsamp = (fluid_list_t *) (genval.uword + 1);
		  break;	/* break out of generator loop */
		}
	      else
		{
		  level = 2;
		  if (gen_valid (genid))
		    {		/* gen valid? */
		      READW (genval.sword, fd);
		      dup = gen_inlist (genid, z->gen);
		    }
		  else
		    skip = TRUE;
		}

	      if (!skip)
		{
		  if (!dup)
		    {		/* if gen ! dup alloc new */
		      g = FLUID_NEW (SFGen);
		      p3->data = g;
		      g->id = genid;
		    }
		  else
		    {
		      g = (SFGen *) (dup->data);
		      drop = TRUE;
		    }
		  g->amount = genval;
		}
	      else
		{		/* skip this generator */
		  discarded = TRUE;
		  drop = TRUE;
		  FSKIPW (fd);
		}

	      if (!drop)
		p3 = fluid_list_next (p3);	/* next gen */
	      else
		SLADVREM (z->gen, p3);

	    }			/* generator loop */

	  if (level == 3)
	    SLADVREM (z->gen, p3);	/* zone has sample? */
	  else
	    {			/* its a global zone */
	      if (!gzone)
		{
		  gzone = TRUE;

		  /* if global zone is not 1st zone, relocate */
		  if (*hz != p2)
		    {
		      void* save = p2->data;
		      FLUID_LOG (FLUID_WARN,
			_("Instrument \"%s\": Global zone is not first zone"),
			((SFPreset *) (p->data))->name);
		      SLADVREM (*hz, p2);
		      *hz = fluid_list_prepend (*hz, save);
		      continue;
		    }
		}
	      else
		{		/* previous global zone exists, discard */
		  FLUID_LOG (FLUID_WARN,
		    _("Instrument \"%s\": Discarding invalid global zone"),
		    ((SFInst *) (p->data))->name);
		  sfont_zone_delete (sf, hz, (SFZone *) (p2->data));
		}
	    }

	  while (p3)
	    {			/* Kill any zones following a sample */
	      discarded = TRUE;
	      if ((size -= SFGENSIZE) < 0)
		return (gerr (ErrCorr,
		    _("Instrument generator chunk size mismatch")));
	      FSKIP (SFGENSIZE, fd);
	      SLADVREM (z->gen, p3);
	    }

	  p2 = fluid_list_next (p2);	/* next zone */
	}
      if (discarded)
	FLUID_LOG(FLUID_WARN,
	  _("Instrument \"%s\": Some invalid generators were discarded"),
	  ((SFInst *) (p->data))->name);
      p = fluid_list_next (p);
    }

  /* for those non-terminal record cases, grr! */
  if (size == 0)
    return (OK);

  size -= SFGENSIZE;
  if (size != 0)
    return (gerr (ErrCorr, _("IGEN chunk size mismatch")));
  FSKIP (SFGENSIZE, fd);	/* terminal gen */

  return (OK);
}

/* sample header loader */
static int
load_shdr (unsigned int size, SFData * sf, FILE * fd)
{
  unsigned int i;
  SFSample *p;

  if (size % SFSHDRSIZE || size == 0)	/* size is multiple of SHDR size? */
    return (gerr (ErrCorr, _("Sample header has invalid size")));

  size = size / SFSHDRSIZE - 1;
  if (size == 0)
    {				/* at least one sample + term record? */
      FLUID_LOG (FLUID_WARN, _("File contains no samples"));
      FSKIP (SFSHDRSIZE, fd);
      return (OK);
    }

  /* load all sample headers */
  for (i = 0; i < size; i++)
    {
      p = FLUID_NEW (SFSample);
      sf->sample = fluid_list_append (sf->sample, p);
      READSTR (&p->name, fd);
      READD (p->start, fd);
      READD (p->end, fd);	/* - end, loopstart and loopend */
      READD (p->loopstart, fd);	/* - will be checked and turned into */
      READD (p->loopend, fd);	/* - offsets in fixup_sample() */
      READD (p->samplerate, fd);
      READB (p->origpitch, fd);
      READB (p->pitchadj, fd);
      FSKIPW (fd);		/* skip sample link */
      READW (p->sampletype, fd);
      p->samfile = 0;
    }

  FSKIP (SFSHDRSIZE, fd);	/* skip terminal shdr */

  return (OK);
}

/* "fixup" (inst # -> inst ptr) instrument references in preset list */
static int
fixup_pgen (SFData * sf)
{
  fluid_list_t *p, *p2, *p3;
  SFZone *z;
  int i;

  p = sf->preset;
  while (p)
    {
      p2 = ((SFPreset *) (p->data))->zone;
      while (p2)
	{			/* traverse this preset's zones */
	  z = (SFZone *) (p2->data);
	  if ((i = GPOINTER_TO_INT (z->instsamp)))
	    {			/* load instrument # */
	      p3 = fluid_list_nth (sf->inst, i - 1);
	      if (!p3)
		return (gerr (ErrCorr,
		    _("Preset %03d %03d: Invalid instrument reference"),
		    ((SFPreset *) (p->data))->bank,
		    ((SFPreset *) (p->data))->prenum));
	      z->instsamp = p3;
	    }
	  else
	    z->instsamp = NULL;
	  p2 = fluid_list_next (p2);
	}
      p = fluid_list_next (p);
    }

  return (OK);
}

/* "fixup" (sample # -> sample ptr) sample references in instrument list */
static int
fixup_igen (SFData * sf)
{
  fluid_list_t *p, *p2, *p3;
  SFZone *z;
  int i;

  p = sf->inst;
  while (p)
    {
      p2 = ((SFInst *) (p->data))->zone;
      while (p2)
	{			/* traverse instrument's zones */
	  z = (SFZone *) (p2->data);
	  if ((i = GPOINTER_TO_INT (z->instsamp)))
	    {			/* load sample # */
	      p3 = fluid_list_nth (sf->sample, i - 1);
	      if (!p3)
		return (gerr (ErrCorr,
		    _("Instrument \"%s\": Invalid sample reference"),
		    ((SFInst *) (p->data))->name));
	      z->instsamp = p3;
	    }
	  p2 = fluid_list_next (p2);
	}
      p = fluid_list_next (p);
    }

  return (OK);
}

/* convert sample end, loopstart and loopend to offsets and check if valid */
static int
fixup_sample (SFData * sf)
{
  fluid_list_t *p;
  SFSample *sam;

  p = sf->sample;
  while (p)
    {
      sam = (SFSample *) (p->data);

      /* if sample is not a ROM sample and end is over the sample data chunk
         or sam start is greater than 4 less than the end (at least 4 samples) */
      if ((!(sam->sampletype & FLUID_SAMPLETYPE_ROM)
	  && sam->end > sdtachunk_size) || sam->start > (sam->end - 4))
	{
	  FLUID_LOG (FLUID_WARN, _("Sample '%s' start/end file positions are invalid,"
	      " disabling and will not be saved"), sam->name);

	  /* disable sample by setting all sample markers to 0 */
	  sam->start = sam->end = sam->loopstart = sam->loopend = 0;

	  return (OK);
	}
      else if (sam->loopend > sam->end || sam->loopstart >= sam->loopend
	|| sam->loopstart <= sam->start)
	{			/* loop is fowled?? (cluck cluck :) */
	  /* can pad loop by 8 samples and ensure at least 4 for loop (2*8+4) */
	  if ((sam->end - sam->start) >= 20)
	    {
	      sam->loopstart = sam->start + 8;
	      sam->loopend = sam->end - 8;
	    }
	  else
	    {			/* loop is fowled, sample is tiny (can't pad 8 samples) */
	      sam->loopstart = sam->start + 1;
	      sam->loopend = sam->end - 1;
	    }
	}

      /* convert sample end, loopstart, loopend to offsets from sam->start */
      sam->end -= sam->start + 1;	/* marks last sample, contrary to SF spec. */
      sam->loopstart -= sam->start;
      sam->loopend -= sam->start;

      p = fluid_list_next (p);
    }

  return (OK);
}

/*=================================sfont.c========================
  Smurf SoundFont Editor
  ================================================================*/

unsigned short badgen[] = { Gen_Unused1, Gen_Unused2, Gen_Unused3, Gen_Unused4,
  Gen_Reserved1, Gen_Reserved2, Gen_Reserved3, 0
};

unsigned short badpgen[] = { Gen_StartAddrOfs, Gen_EndAddrOfs, Gen_StartLoopAddrOfs,
  Gen_EndLoopAddrOfs, Gen_StartAddrCoarseOfs, Gen_EndAddrCoarseOfs,
  Gen_StartLoopAddrCoarseOfs, Gen_Keynum, Gen_Velocity,
  Gen_EndLoopAddrCoarseOfs, Gen_SampleModes, Gen_ExclusiveClass,
  Gen_OverrideRootKey, 0
};

void
sfont_close (SFData * sf)
{
  if (sf->sffd)
    fclose (sf->sffd);

  fsd_sfont_free_data (sf);
}

/* delete a sound font structure */
void
fsd_sfont_free_data (SFData * sf)
{
  fluid_list_t *p, *p2;

  if (sf->fname)
    free (sf->fname);

  p = sf->info;
  while (p)
    {
      free (p->data);
      p = fluid_list_next (p);
    }
  delete_fluid_list(sf->info);
  sf->info = NULL;

  p = sf->preset;
  while (p)
    {				/* loop over presets */
      p2 = ((SFPreset *) (p->data))->zone;
      while (p2)
	{			/* loop over preset's zones */
	  sfont_free_zone (p2->data);
	  p2 = fluid_list_next (p2);
	}			/* free preset's zone list */
      delete_fluid_list (((SFPreset *) (p->data))->zone);
      FLUID_FREE (p->data);	/* free preset chunk */
      p = fluid_list_next (p);
    }
  delete_fluid_list (sf->preset);
  sf->preset = NULL;

  p = sf->inst;
  while (p)
    {				/* loop over instruments */
      p2 = ((SFInst *) (p->data))->zone;
      while (p2)
	{			/* loop over inst's zones */
	  sfont_free_zone (p2->data);
	  p2 = fluid_list_next (p2);
	}			/* free inst's zone list */
      delete_fluid_list (((SFInst *) (p->data))->zone);
      FLUID_FREE (p->data);
      p = fluid_list_next (p);
    }
  delete_fluid_list (sf->sample);
  sf->sample = NULL;
}

/* free all elements of a zone (Preset or Instrument) */
void
sfont_free_zone (SFZone * zone)
{
  fluid_list_t *p;

  if (!zone)
    return;

  p = zone->gen;
  while (p)
    {				/* Free gen chunks for this zone */
      if (p->data)
	FLUID_FREE (p->data);
      p = fluid_list_next (p);
    }
  delete_fluid_list (zone->gen);	/* free genlist */

  p = zone->mod;
  while (p)
    {				/* Free mod chunks for this zone */
      if (p->data)
	FLUID_FREE (p->data);
      p = fluid_list_next (p);
    }
  delete_fluid_list (zone->mod);	/* free modlist */

  FLUID_FREE (zone);	/* free zone chunk */
}

/* preset sort function, first by bank, then by preset # */
int
sfont_preset_compare_func (void* a, void* b)
{
  int aval, bval;

  aval = (int) (((SFPreset *) a)->bank) << 16 | ((SFPreset *) a)->prenum;
  bval = (int) (((SFPreset *) b)->bank) << 16 | ((SFPreset *) b)->prenum;

  return (aval - bval);
}

/* delete zone from zone list */
void
sfont_zone_delete (SFData * sf, fluid_list_t ** zlist, SFZone * zone)
{
  *zlist = fluid_list_remove (*zlist, (void*) zone);
  sfont_free_zone (zone);
}

/* Find generator in gen list */
fluid_list_t *
gen_inlist (int gen, fluid_list_t * genlist)
{				/* is generator in gen list? */
  fluid_list_t *p;

  p = genlist;
  while (p)
    {
      if (p->data == NULL)
	return (NULL);
      if (gen == ((SFGen *) p->data)->id)
	break;
      p = fluid_list_next (p);
    }
  return (p);
}

/* check validity of instrument generator */
int
gen_valid (int gen)
{				/* is generator id valid? */
  int i = 0;

  if (gen > Gen_MaxValid)
    return (FALSE);
  while (badgen[i] && badgen[i] != gen)
    i++;
  return (badgen[i] == 0);
}

/* check validity of preset generator */
int
gen_validp (int gen)
{				/* is preset generator valid? */
  int i = 0;

  if (!gen_valid (gen))
    return (FALSE);
  while (badpgen[i] && badpgen[i] != (unsigned short) gen)
    i++;
  return (badpgen[i] == 0);
}

/*================================util.c===========================*/

/* Logging function, returns FAIL to use as a return value in calling funcs */
int
gerr (int ev, char * fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  vprintf(fmt, args);
  va_end (args);

  printf("\n");

  return (FAIL);
}

int
safe_fread (void *buf, int count, FILE * fd)
{
  if (fread (buf, count, 1, fd) != 1)
    {				/* size_t = count, nmemb = 1 */
      if (feof (fd))
	gerr (ErrEof, _("EOF while attemping to read %d bytes"), count);
      else
	FLUID_LOG (FLUID_ERR, _("File read failed"));
      return (FAIL);
    }
  return (OK);
}

int
safe_fseek (FILE * fd, long ofs, int whence)
{
  if (fseek (fd, ofs, whence) == -1) {
    FLUID_LOG (FLUID_ERR, _("File seek failed with offset = %ld and whence = %d"), ofs, whence);
    return (FAIL);
  }
  return (OK);
}

void *
safe_malloc (size_t size)
{
  void *ptr;

  if (!(ptr = malloc (size)))
    FLUID_LOG (FLUID_ERR, _("Attempted to allocate %d bytes"), (int) size);
  return (ptr);
}

