/* FluidSynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
 * Portions of this file may have come from Chris Cannam and Steve
 * Harris's public domain DSSI example code.
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

#ifndef _FLUIDSYNTH_DSSI_H
#define _FLUIDSYNTH_DSSI_H

#include <ladspa.h>
#include "fluidsynth_priv.h"   /* for FLUID_BUFSIZE */

/* ==== debugging ==== */

/* DSSI interface debugging -- choose one: */
#define DEBUG_DSSI(fmt...)
// #define DEBUG_DSSI(fmt...) { fprintf(stderr, fmt); }

/* audio debugging -- define if desired: */
// #define DEBUG_AUDIO

/* ==== end of debugging ==== */

#define FSD_MAX_CHANNELS  256  /* derived from FluidSynth's virtual channel limit */

typedef enum {
    PORT_OUTPUT_LEFT     = 0,
    PORT_OUTPUT_RIGHT    = 1,
    FSD_PORTS_COUNT      = 2
} PortNumbers;

struct fsd_port_descriptor {
    LADSPA_PortDescriptor          port_descriptor;
    char *                         name;
    LADSPA_PortRangeHintDescriptor hint_descriptor;
    LADSPA_Data                    lower_bound;
    LADSPA_Data                    upper_bound;
};

typedef struct _fsd_instance_t fsd_instance_t;
typedef struct _fsd_sfont_t fsd_sfont_t;
typedef struct _fsd_synth_t fsd_synth_t;

struct _fsd_instance_t {
    int             channel;
    fsd_sfont_t    *soundfont;
    LADSPA_Data    *output_l;
    LADSPA_Data    *output_r;
    LADSPA_Data    *tmpbuf_l;
    LADSPA_Data    *tmpbuf_r;
    LADSPA_Data     tmpbuf_unaligned[2 * FLUID_BUFSIZE + 4];
};

struct _fsd_sfont_t {
    fsd_sfont_t             *next;
    char                    *path;
    int                      sfont_id;
    int                      ref_count;
    int                      program_count;
    DSSI_Program_Descriptor *programs;
};

struct _fsd_synth_t {
    pthread_mutex_t   mutex;
    int               mutex_grab_failed;
    unsigned long     instance_count;
    unsigned long     burst_remains;
    fluid_settings_t *fluid_settings;
    fluid_synth_t    *fluid_synth;
    fsd_sfont_t      *soundfonts;
    float             gain;
    fsd_instance_t   *channel_map[FSD_MAX_CHANNELS];
};

#endif /* _FLUIDSYNTH_DSSI_H */
