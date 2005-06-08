/* FluidSynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2005 Sean Bolton and others.
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
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _FLUIDSYNTH_DSSI_H
#define _FLUIDSYNTH_DSSI_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ladspa.h>

#include <fluidsynth.h>

/* ==== debugging ==== */

/* DSSI interface debugging -- choose one: */
#define DEBUG_DSSI(fmt...)
// #define DEBUG_DSSI(fmt...) { fprintf(stderr, fmt); }

/* audio debugging -- define if desired: */
// #define DEBUG_AUDIO

/* ==== end of debugging ==== */

#define FSD_MAX_CHANNELS  255  /* FluidSynth's virtual channel limit (n.b. NO_CHANNEL in fluid_voice.h) */

/* FSD_CHANNEL_COUNT is the number of channels FluidSynth-DSSI tells
 * libfluidsynth to create at startup.  It can be up to FSD_MAX_CHANNELS,
 * but be aware that for each channel, libfluidsynth will zero and copy out
 * two buffers every process cycle, so it's best to set FSD_CHANNEL_COUNT
 * to no more than the maximum number of simultaneous FluidSynth-DSSI
 * instances you'll need. */
#define FSD_CHANNEL_COUNT  16

/* -FIX- These should be in a header file shared with FluidSynth-DSSI_gtk.c: */
#define FSD_MAX_POLYPHONY     256
#define FSD_DEFAULT_POLYPHONY 256

#define FSD_MAX_BURST_SIZE  512

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
    int             pending_preset_change;
    fsd_sfont_t    *soundfont;
    LADSPA_Data    *output_l;
    LADSPA_Data    *output_r;
#ifndef NWRITE_FLOAT_WORKS_CORRECTLY
    LADSPA_Data    *tmpbuf_l;
    LADSPA_Data    *tmpbuf_r;
#endif
};

struct _fsd_sfont_t {
    fsd_sfont_t             *next;
    char                    *path;
    int                      sfont_id;
    int                      ref_count;
    int                      preset_count;
    DSSI_Program_Descriptor *presets;
};

struct _fsd_synth_t {
    pthread_mutex_t   mutex;
    int               mutex_grab_failed;
    unsigned long     instance_count;
#ifndef NWRITE_FLOAT_WORKS_CORRECTLY
    unsigned long     fluid_bufsize;
    unsigned long     burst_remains;
#endif
    char             *project_directory;
    fluid_settings_t *fluid_settings;
    fluid_synth_t    *fluid_synth;
    fsd_sfont_t      *soundfonts;
    float             gain;
#ifdef USE_AUGMENTED_FLUIDSYNTH_API
    int               polyphony;
#endif
    fsd_instance_t   *channel_map[FSD_CHANNEL_COUNT];
    LADSPA_Data       bit_bucket[FSD_MAX_BURST_SIZE];
    LADSPA_Data      *fx_buckets[2];
};

#endif /* _FLUIDSYNTH_DSSI_H */

