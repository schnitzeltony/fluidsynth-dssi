/* FluidSynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Portions of this file came from FluidSynth, copyright (C) 2003
 * Peter Hanappe and others, which in turn borrowed code from GLIB,
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _FLUIDSYNTH_DSSI_GTK_H
#define _FLUIDSYNTH_DSSI_GTK_H

/* ==== version check ==== */

#include <fluidsynth.h>

#if (FLUIDSYNTH_VERSION_MAJOR != 1) || (FLUIDSYNTH_VERSION_MINOR != 0) || (FLUIDSYNTH_VERSION_MICRO != 3)
#warning  ----------------------------------------------------------
#warning  This code uses structures and functions from libfluidsynth
#warning  that are not part of its standard API.  This has only been
#warning  tested with FluidSynth version 1.0.3, which is not the
#warning  you appear to have installed.  Proceed at your own risk!
#warning  ----------------------------------------------------------
#endif

/* ==== debugging ==== */

/* DSSI interface debugging -- choose one: */
#define DEBUG_DSSI(fmt...)
// #define DEBUG_DSSI(fmt...) { fprintf(stderr, fmt); }

/* ==== things we need from FluidSynth, that aren't in the API ==== */

/* from fluid_list.h: */

typedef struct _fluid_list_t fluid_list_t;

struct _fluid_list_t
{
  void* data;
  fluid_list_t *next;
};

#define fluid_list_next(slist)  ((slist) ? (((fluid_list_t *)(slist))->next) : NULL)

/* from fluid_defsfont.h: */

#include <stdio.h>

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

SFData *sfload_file (const char * fname);
void    sfont_free_data (SFData * sf);
int     sfont_preset_compare_func (const void* a, const void* b);

/* ==== FluidSynth-DSSI_gtk prototypes ==== */

int  lo_server_get_socket_fd(lo_server *server);
int  lo_server_set_nonblocking(lo_server *server);
int  osc_debug_handler(const char *path, const char *types, lo_arg **argv,
                       int argc, lo_message msg, void *user_data);
int  osc_action_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, lo_message msg, void *user_data);
int  osc_configure_handler(const char *path, const char *types, lo_arg **argv,
                           int argc, lo_message msg, void *user_data);
int  osc_control_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, lo_message msg, void *user_data);
int  osc_program_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, lo_message msg, void *user_data);
void osc_data_on_socket_callback(gpointer data, gint source,
                                 GdkInputCondition condition);
gint on_delete_event_wrapper(GtkWidget *widget, GdkEvent *event,
                             gpointer data);
void on_select_soundfont_button_press(GtkWidget *widget, gpointer data);
int  load_soundfont(char *filename);
void on_file_selection_ok(GtkWidget *widget, gpointer data);
void on_file_selection_cancel(GtkWidget *widget, gpointer data);
void on_preset_selection(GtkWidget *clist, gint row, gint column,
                         GdkEventButton *event, gpointer data);
void on_test_note_slider_change(GtkWidget *widget, gpointer data);
void on_test_note_button_press(GtkWidget *widget, gpointer data);
void on_notice_dismiss(GtkWidget *widget, gpointer data);
void update_from_program_select(int bank, int program);
void rebuild_preset_clist(SFData *sfdata);
GtkWidget *create_main_window(const char *tag);
GtkWidget *create_file_selection(const char *tag);
GtkWidget *create_notice_window(const char *tag);
void create_windows(const char *instance_tag);

#endif /* _FLUIDSYNTH_DSSI_GTK_H */

