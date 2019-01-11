/* FluidSynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004-2008, 2012 Sean Bolton and others.
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#define _DEFAULT_SOURCE 1
#define _ISOC99_SOURCE  1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>

#include <glib.h>
#include <ladspa.h>
#include <dssi.h>

#include "fluidsynth.h"

#include "fluidsynth-dssi.h"

/* in locate_soundfont.c: */
char *fsd_locate_soundfont_file(const char *origpath,
                                const char *projectDirectory);

static LADSPA_Descriptor  *fsd_LADSPA_descriptor = NULL;
static DSSI_Descriptor    *fsd_DSSI_descriptor = NULL;

static fsd_synth_t         fsd_synth;

static fsd_settings_t      fsd_settings;

static LADSPA_Data **l_outputs = NULL;
static LADSPA_Data **r_outputs = NULL;


struct fsd_port_descriptor fsd_port_description[FSD_PORTS_COUNT] = {
#define PD_OUT     (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO)
#define PD_IN      (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL)
    { PD_OUT, "Output Left",              0,               0.0f,     0.0f },
    { PD_OUT, "Output Right",             0,               0.0f,     0.0f },
#undef PD_OUT
#undef PD_IN
};

static void
fsd_all_voices_off(void);

static void
fsd_cleanup(LADSPA_Handle handle);

static void
fsd_run_multiple_synths(unsigned long instance_count, LADSPA_Handle *handles,
                        unsigned long sample_count, snd_seq_event_t **events,
                        unsigned long *event_counts);

/* ---- FluidSynth helper functions ---- */

/*
 * fsd_chan_all_voices_off
 *
 * turn off all voices on channel immediately
 */
static inline void
fsd_chan_all_voices_off(int channel)
{
    fluid_synth_cc(fsd_synth.fluid_synth, channel, 0x78, 0);  /* 0x78 = MIDI 'all sound off' control change */
}

/*
 * fsd_all_voices_off
 *
 * turn off all voices on all channels immediately
 */
static void
fsd_all_voices_off(void)
{
    int i;

    for (i = 0; i < fsd_settings.channel_count; i++) {
        fsd_chan_all_voices_off(i);
    }
}

/* ---- mutual exclusion ---- */

static inline int
fsd_mutex_trylock(void)
{
    int rc;

    /* Attempt the mutex lock */
    rc = pthread_mutex_trylock(&fsd_synth.mutex);
    if (rc) {
        fsd_synth.mutex_grab_failed = 1;
        return rc;
    }
    /* Clean up if a previous mutex grab failed */
    if (fsd_synth.mutex_grab_failed) {
        fsd_all_voices_off();
        fsd_synth.mutex_grab_failed = 0;
    }
    return 0;
}

static inline int
fsd_mutex_lock(void)
{
    return pthread_mutex_lock(&fsd_synth.mutex);
}

static inline int
fsd_mutex_unlock(void)
{
    return pthread_mutex_unlock(&fsd_synth.mutex);
}

/* ---- soundfont handling ---- */

/*
 * fsd_find_loaded_soundfont
 */
fsd_sfont_t *
fsd_find_loaded_soundfont(const char *path)
{
    fsd_sfont_t *sfont;

    /* check if we already have the soundfont loaded */
    sfont = fsd_synth.soundfonts;
    while (sfont) {
        if (!strcmp(path, sfont->path)) {
            return sfont;
        }
        sfont = sfont->next;
    }
    return NULL;
}

/*
 * fsd_get_soundfont
 */
fsd_sfont_t *
fsd_get_soundfont(const char *path)
{
    fsd_sfont_t *sfont;
    int palloc;
    fluid_sfont_t *fluid_sfont;
#if FLUIDSYNTH_VERSION_MAJOR < 2
    fluid_preset_t preset;
#else
    fluid_preset_t *preset;
#endif

    /* soundfont already loaded? */
    sfont = fsd_find_loaded_soundfont(path);
    if (sfont) {
        sfont->ref_count++;
        DEBUG_DSSI("fsd: soundfont %d refcount now %d\n", sfont->sfont_id, sfont->ref_count);
        return sfont;
    }

    /* nope, so load it */
    sfont = (fsd_sfont_t *)calloc(1, sizeof(fsd_sfont_t));
    if (!sfont) {
        return NULL;
    }
    sfont->path = strdup(path);
    if (!sfont->path) {
        free(sfont);
        return NULL;
    }
    sfont->sfont_id = fluid_synth_sfload(fsd_synth.fluid_synth, path, 0);
    if (sfont->sfont_id == -1) {
        free(sfont->path);
        free(sfont);
        return NULL;
    }
    sfont->ref_count = 1;

    /* enumerate presets */
    sfont->preset_count = 0;
    palloc = 256;
    sfont->presets = (DSSI_Program_Descriptor *)malloc(palloc * sizeof(DSSI_Program_Descriptor));
    if (!sfont->presets) {
        fluid_synth_sfunload(fsd_synth.fluid_synth, sfont->sfont_id, 0);
        free(sfont->path);
        free(sfont);
        return NULL;
    }
    fluid_sfont = fluid_synth_get_sfont_by_id(fsd_synth.fluid_synth, sfont->sfont_id);
#if FLUIDSYNTH_VERSION_MAJOR < 2
    fluid_sfont->iteration_start(fluid_sfont);
    while (fluid_sfont->iteration_next(fluid_sfont, &preset)) {
#else
    fluid_sfont_iteration_start(fluid_sfont);
    while ((preset = fluid_sfont_iteration_next(fluid_sfont))) {
#endif
        if (sfont->preset_count == palloc) {
            palloc *= 2;
            sfont->presets = (DSSI_Program_Descriptor *)realloc(sfont->presets,
                                                                palloc * sizeof(DSSI_Program_Descriptor));
            if (!sfont->presets) {
                fluid_synth_sfunload(fsd_synth.fluid_synth, sfont->sfont_id, 0);
                free(sfont->path);
                free(sfont);
                return NULL;
            }
        }
#if FLUIDSYNTH_VERSION_MAJOR < 2
        sfont->presets[sfont->preset_count].Bank = preset.get_banknum(&preset);
        sfont->presets[sfont->preset_count].Program = preset.get_num(&preset);
        sfont->presets[sfont->preset_count].Name = preset.get_name(&preset);
#else
        sfont->presets[sfont->preset_count].Bank = fluid_preset_get_banknum(preset);
        sfont->presets[sfont->preset_count].Program = fluid_preset_get_num(preset);
        sfont->presets[sfont->preset_count].Name = fluid_preset_get_name(preset);
#endif
        sfont->preset_count++;
    }

    /* add it to soundfont list */
    sfont->next = fsd_synth.soundfonts;
    fsd_synth.soundfonts = sfont;

    DEBUG_DSSI("fsd: soundfont '%s' loaded as sfont_id %d (refcount 1, %d presets)\n", path, sfont->sfont_id, sfont->preset_count);
    return sfont;
}

/*
 * fsd_release_soundfont
 */
void
fsd_release_soundfont(fsd_sfont_t *sfont)
{
    if (--sfont->ref_count == 0) {
        fsd_sfont_t *prev;

        DEBUG_DSSI("fsd: freeing soundfont %d\n", sfont->sfont_id);

        /* remove soundfont from list */
        if (fsd_synth.soundfonts == sfont) {
            fsd_synth.soundfonts = sfont->next;
        } else {
            prev = fsd_synth.soundfonts;
            while (prev->next != sfont)
                prev = prev->next;
            prev->next = sfont->next;
        }

        /* I haven't figured out how or where, but it seems that when calling
         * fluid_synth_sfunload, fluidsynth turns off notes that are using the
         * soundfont being unloaded, so I'm hoping we don't need to do that
         * here.  Please correct me if I'm wrong about that.... */

        /* free soundfont */
        fluid_synth_sfunload(fsd_synth.fluid_synth, sfont->sfont_id, 0);
        free(sfont->presets);
        free(sfont->path);
        free(sfont);
    } else {
        DEBUG_DSSI("fsd: soundfont %d refcount now %d\n", sfont->sfont_id, sfont->ref_count);
    }
}

/* ---- LADSPA interface ---- */

/*
 * fsd_instantiate
 *
 * implements LADSPA (*instantiate)()
 */
static LADSPA_Handle
fsd_instantiate(const LADSPA_Descriptor *descriptor, unsigned long sample_rate)
{
    fsd_instance_t *instance;
    int i, configval;
    gchar *configdirname, *configfilename;
    GKeyFile *key_file;

    /* fsd_settings.channel_count unset on first call */
    if (fsd_synth.instance_count != 0 &&
            /* refuse another instantiation if we've reached out limit */
            fsd_synth.instance_count == fsd_settings.channel_count) {
        return NULL;
    }

    /* initialize FluidSynth if this is our first instance */
    if (fsd_synth.instance_count == 0) {

        /* initialize the settings */
        if (!fsd_synth.fluid_settings &&
            !(fsd_synth.fluid_settings = new_fluid_settings())) {
            return NULL;
        }

        /* configfile settings defaults */
        fsd_settings.channel_count = FSD_CHANNEL_COUNT_DEFAULT;
        fluid_settings_getint(fsd_synth.fluid_settings, "audio.realtime-prio", &fsd_settings.realtime_prio);
        fluid_settings_getint(fsd_synth.fluid_settings, "synth.cpu-cores", &fsd_settings.thread_count);
#if FLUIDSYNTH_VERSION_MAJOR >= 2
        fluid_settings_getint(fsd_synth.fluid_settings, "synth.dynamic-sample-loading", &fsd_settings.dynamic_sample_loading);
#endif
        configdirname = g_build_filename(g_get_user_config_dir(), "fluidsynth-dssi", NULL);
        configfilename = g_build_filename(configdirname, "fluidsynth-dssi.conf", NULL);
        key_file = g_key_file_new();
        /* No config file yet: create one with defaults */
        if (!g_key_file_load_from_file(
                key_file,
                configfilename,
                G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
            NULL)) {
            g_key_file_set_integer(key_file, "audio", "realtime-prio", fsd_settings.realtime_prio);
            g_key_file_set_integer(key_file, "synth", "maxchannels", fsd_settings.channel_count);
            g_key_file_set_integer(key_file, "synth", "parallel-threads", fsd_settings.thread_count);
#if FLUIDSYNTH_VERSION_MAJOR >= 2
            g_key_file_set_integer(key_file, "synth", "dynamic-sample-loading", fsd_settings.dynamic_sample_loading);
#endif
            g_mkdir_with_parents(configdirname, 0700);
            g_key_file_save_to_file(key_file, configfilename, NULL);
        /* config file available: get settings */
        } else {
            configval = g_key_file_get_integer(key_file, "audio", "realtime-prio", NULL);
            if (configval > 0)
                fsd_settings.realtime_prio = configval;
            configval = g_key_file_get_integer(key_file, "synth", "maxchannels", NULL);
            if (configval > 0)
                fsd_settings.channel_count = configval;
            configval = g_key_file_get_integer(key_file, "synth", "parallel-threads", NULL);
            if (configval > 0)
                fsd_settings.thread_count = configval;
#if FLUIDSYNTH_VERSION_MAJOR >= 2
            configval = g_key_file_get_integer(key_file, "synth", "dynamic-sample-loading", NULL);
            if (configval > 0)
                fsd_settings.dynamic_sample_loading = configval;
#endif
        }

        g_key_file_free(key_file);
        g_free(configfilename);
        g_free(configdirname);

        /* setup buffers from persitant settings */
        fsd_synth.channel_map = (fsd_instance_t **)calloc(fsd_settings.channel_count, sizeof(fsd_instance_t *));
        l_outputs = (LADSPA_Data **) calloc(fsd_settings.channel_count, sizeof(LADSPA_Data *));
        r_outputs = (LADSPA_Data **) calloc(fsd_settings.channel_count, sizeof(LADSPA_Data *));

        /* set appropriate settings here */
        fluid_settings_setnum(fsd_synth.fluid_settings, "synth.sample-rate", sample_rate);
        fluid_settings_setint(fsd_synth.fluid_settings, "synth.midi-channels", fsd_settings.channel_count);
        fluid_settings_setint(fsd_synth.fluid_settings, "synth.audio-channels", fsd_settings.channel_count);
        fluid_settings_setint(fsd_synth.fluid_settings, "synth.audio-groups", fsd_settings.channel_count);
        fluid_settings_setint(fsd_synth.fluid_settings, "audio.realtime-prio", fsd_settings.realtime_prio);
        fluid_settings_setint(fsd_synth.fluid_settings, "synth.cpu-cores", fsd_settings.thread_count);
#if FLUIDSYNTH_VERSION_MAJOR >= 2
        fluid_settings_setint(fsd_synth.fluid_settings, "synth.dynamic-sample-loading", fsd_settings.dynamic_sample_loading);
#endif
        fsd_synth.polyphony = FSD_MAX_POLYPHONY;
        fluid_settings_setint(fsd_synth.fluid_settings, "synth.polyphony", fsd_synth.polyphony);
        fluid_settings_setint(fsd_synth.fluid_settings, "synth.reverb.active", 0);
        fluid_settings_setint(fsd_synth.fluid_settings, "synth.chorus.active", 0);

        /* initialize the FluidSynth engine */
        if (!fsd_synth.fluid_synth &&
            !(fsd_synth.fluid_synth = new_fluid_synth(fsd_synth.fluid_settings))) {
            delete_fluid_settings(fsd_synth.fluid_settings);
            return NULL;
        }

        /* other module-wide initialization */
        fsd_synth.project_directory = NULL;
        fsd_synth.gain = -1.0f;
    }

    instance = (fsd_instance_t *)calloc(1, sizeof(fsd_instance_t));
    if (!instance) {
        fsd_synth.instance_count++;
        fsd_cleanup(NULL);
        return NULL;
    }
    /* find a free channel */
    for (i = 0; i < fsd_settings.channel_count; i++) {
        if (fsd_synth.channel_map[i] == NULL) {
            fsd_synth.channel_map[i] = instance;
            instance->channel = i;
            break;
        }
    }

    instance->pending_preset_change = -1;
    instance->soundfont = NULL;

    fsd_synth.instance_count++;

    return (LADSPA_Handle)instance;
}

/*
 * fsd_connect_port
 *
 * implements LADSPA (*connect_port)()
 */
static void
fsd_connect_port(LADSPA_Handle handle, unsigned long port, LADSPA_Data *data)
{
    fsd_instance_t *instance = (fsd_instance_t *)handle;

    switch (port) {
      case PORT_OUTPUT_LEFT:      instance->output_l = data;  break;
      case PORT_OUTPUT_RIGHT:     instance->output_r = data;  break;

      default:
        break;
    }
}

/*
 * fsd_activate
 *
 * implements LADSPA (*activate)()
 */
static void
fsd_activate(LADSPA_Handle handle)
{
    // fsd_instance_t *instance = (fsd_instance_t *)handle;

    /* we're already ready to go.... */
}

/*
 * fsd_ladspa_run
 *
 * since we can't implement LADSPA (*run)() safely in a way that supports
 * more than one instance, we just return immediately
 */
static void
fsd_ladspa_run(LADSPA_Handle handle, unsigned long sample_count)
{
    return;
}

// optional:
//  void (*run_adding)(LADSPA_Handle Instance,
//                     unsigned long SampleCount);
//  void (*set_run_adding_gain)(LADSPA_Handle Instance,
//                              LADSPA_Data   Gain);

/*
 * fsd_deactivate
 *
 * implements LADSPA (*deactivate)()
 */
void
fsd_deactivate(LADSPA_Handle handle)
{
    fsd_instance_t *instance = (fsd_instance_t *)handle;

    /* stop all voices on channel immediately */
    fsd_chan_all_voices_off(instance->channel);
}

/*
 * fsd_cleanup
 *
 * implements LADSPA (*cleanup)()
 */
static void
fsd_cleanup(LADSPA_Handle handle)
{
    fsd_instance_t *instance = (fsd_instance_t *)handle;

    if (instance) {
        /* release the soundfont */
        if (instance->soundfont) {
            fsd_release_soundfont(instance->soundfont);
            instance->soundfont = NULL;
        }

        /* free the channel */
        fsd_synth.channel_map[instance->channel] = NULL;

    }

    /* if there are no more instances, take down FluidSynth */
    if (--fsd_synth.instance_count == 0) {
        while (fsd_synth.soundfonts) {
            fsd_sfont_t *next = fsd_synth.soundfonts->next;
            fluid_synth_sfunload(fsd_synth.fluid_synth, fsd_synth.soundfonts->sfont_id, 0);
            free(fsd_synth.soundfonts->presets);
            free(fsd_synth.soundfonts->path);
            free(fsd_synth.soundfonts);
            fsd_synth.soundfonts = next;
        }
        delete_fluid_synth(fsd_synth.fluid_synth);
        delete_fluid_settings(fsd_synth.fluid_settings);
        fsd_synth.fluid_synth = NULL;
        fsd_synth.fluid_settings = NULL;
        free(fsd_synth.channel_map);
        fsd_synth.channel_map = NULL;
        free(l_outputs);
        l_outputs = NULL;
        free(r_outputs);
        r_outputs = NULL;
    }
    free(instance);
}

/* ---- DSSI interface ---- */

/*
 * dssi_configure_message
 */
char *
dssi_configure_message(const char *fmt, ...)
{
    va_list args;
    char buffer[256];

    va_start(args, fmt);
    vsnprintf(buffer, 256, fmt, args);
    va_end(args);
    return strdup(buffer);
}

/*
 * fsd_configure
 *
 * implements DSSI (*configure)()
 */
char *
fsd_configure(LADSPA_Handle handle, const char *key, const char *value)
{
    /* Some of our configuration options are global to the plugin, yet DSSI
     * configure() calls are per-instance.  So for those global options, we'll
     * have our UI send the same configuration message to each instance, and
     * we'll check each call here to see if it repeats the current setting,
     * before acting upon it. */

    fsd_instance_t *instance = (fsd_instance_t *)handle;
    int have_mutex_lock = 0;

    DEBUG_DSSI("fsd %d: fsd_configure called with '%s' and '%s'\n", instance->channel, key, value);

    if (!strcmp(key, "load")) {

        char *sfpath = fsd_locate_soundfont_file(value, fsd_synth.project_directory);

        if (!sfpath)
            return dssi_configure_message("error: could not find soundfont '%s'", value);

        if (instance->soundfont &&
            !strcmp(sfpath, instance->soundfont->path)) {
            free(sfpath);
            return NULL;  /* soundfont already loaded */
        }

        /* avoid grabbing the mutex if possible */
        if ((instance->soundfont && instance->soundfont->ref_count < 2) /* if current soundfont needs unloading */
            || !fsd_find_loaded_soundfont(sfpath)) {                    /*    or requested soundfont not loaded */

            fsd_mutex_lock();
            have_mutex_lock = 1;
        }

        if (instance->soundfont) {
            fsd_release_soundfont(instance->soundfont);
            if (have_mutex_lock) {
                instance->soundfont = NULL;
            }
        }

        instance->soundfont = fsd_get_soundfont(sfpath);
        if (instance->soundfont) {
            instance->pending_preset_change = instance->soundfont->preset_count ? 0 : -1;
        }

        if (have_mutex_lock) {
            fsd_mutex_unlock();
        }

        if (instance->soundfont) {
            if (!strcmp(value, sfpath)) {
                free(sfpath);
                return NULL;  /* success */
            } else {
                char *rv = dssi_configure_message("warning: soundfont '%s' not "
                                                  "found, loaded '%s' instead",
                                                  value, sfpath);
                free(sfpath);
                return rv;
            }
        } else {
            free(sfpath);
            return dssi_configure_message("error: could not load soundfont '%s'", value);
        }

    } else if (!strcmp(key, DSSI_GLOBAL_CONFIGURE_PREFIX "gain")) {

        float new_gain = atof(value); /* -FIX- locale-dependent, may fail if sender isn't using similar locale */

        if (new_gain < 0.0000001f || new_gain > 10.0f) {
            return dssi_configure_message("error: out-of-range gain '%s'", value);
        }
        if (new_gain == fsd_synth.gain) {
            return NULL;  /* gain already set at new_gain */
        }

        fsd_mutex_lock();

        fluid_synth_set_gain(fsd_synth.fluid_synth, new_gain);

        fsd_mutex_unlock();

        fsd_synth.gain = new_gain;
        
        return NULL;

    } else if (!strcmp(key, DSSI_GLOBAL_CONFIGURE_PREFIX "polyphony")) {

        float new_polyphony = atol(value);

        if (new_polyphony < 1 || new_polyphony > FSD_MAX_POLYPHONY) {
            return dssi_configure_message("error: out-of-range polyphony '%s'", value);
        }
        if (new_polyphony == fsd_synth.polyphony) {
            return NULL;  /* polyphony already set at new_polyphony */
        }

        fsd_mutex_lock();

        fluid_synth_set_polyphony(fsd_synth.fluid_synth, new_polyphony);

        fsd_mutex_unlock();

        fsd_synth.polyphony = new_polyphony;
        
        return NULL;

    } else if (!strcmp(key, DSSI_PROJECT_DIRECTORY_KEY)) {

        if (fsd_synth.project_directory) {
            free(fsd_synth.project_directory);
        }
        if (value) {
            fsd_synth.project_directory = strdup(value);
        } else {
            fsd_synth.project_directory = NULL;
        }

        return NULL;

    }

    return strdup("error: unrecognized configure key");
}

/*
 * fsd_get_program
 *
 * implements DSSI (*get_program)()
 */
const DSSI_Program_Descriptor *
fsd_get_program(LADSPA_Handle handle, unsigned long index)
{
    fsd_instance_t *instance = (fsd_instance_t *)handle;

    DEBUG_DSSI("fsd %d: fsd_get_program called with %lu\n", instance->channel, index);

    if (!instance->soundfont || index >= instance->soundfont->preset_count) {
        return NULL;
    }

    return &instance->soundfont->presets[index];
}

/*
 * fsd_select_program
 *
 * implements DSSI (*select_program)()
 */
void
fsd_select_program(LADSPA_Handle handle, unsigned long bank,
                   unsigned long program)
{
    fsd_instance_t *instance = (fsd_instance_t *)handle;
    int preset;

    DEBUG_DSSI("fsd %d: fsd_select_program called with %lu and %lu\n", instance->channel, bank, program);

    if (!instance->soundfont)
        return;

    /* ignore invalid program requests */
    for (preset = 0; preset < instance->soundfont->preset_count; preset++) {
        if (instance->soundfont->presets[preset].Bank == bank &&
            instance->soundfont->presets[preset].Program == program)
            break;
    }
    if (preset == instance->soundfont->preset_count)
        return;

    /* Attempt the mutex, return if lock fails. */
    if (fsd_mutex_trylock()) {
        instance->pending_preset_change = preset;
        return;
    }
    fluid_synth_program_select(fsd_synth.fluid_synth, instance->channel,
                               instance->soundfont->sfont_id, bank, program);

    fsd_mutex_unlock();
}

/*
 * fsd_get_midi_controller
 *
 * implements DSSI (*get_midi_controller_for_port)()
 */
int
fsd_get_midi_controller(LADSPA_Handle handle, unsigned long port)
{
    // fsd_instance_t *instance = (fsd_instance_t *)handle;

    // DEBUG_DSSI("fsd %d: fsd_get_midi_controller called for port %lu\n", instance->channel, port);
    // switch (port) {
    //   case PORT_VOLUME:
    //     return DSSI_CC(7);
    //   default:
    //     break;
    // }

    return DSSI_NONE;
}

// optional:
//    void (*run_synth)(LADSPA_Handle    Instance,
//                      unsigned long    SampleCount,
//                      snd_seq_event_t *Events,
//                      unsigned long    EventCount);
//    void (*run_synth_adding)(LADSPA_Handle    Instance,
//                             unsigned long    SampleCount,
//                             snd_seq_event_t *Events,
//                             unsigned long    EventCount);
    
/*
 * fsd_handle_pending_preset_change
 */
static inline void
fsd_handle_pending_preset_change(fsd_instance_t *instance)
{
    int preset = instance->pending_preset_change;

    fluid_synth_program_select(fsd_synth.fluid_synth,
                               instance->channel,
                               instance->soundfont->sfont_id,
                               instance->soundfont->presets[preset].Bank,
                               instance->soundfont->presets[preset].Program);
}

/*
 * fsd_handle_event
 */
static inline void
fsd_handle_event(fsd_instance_t *instance, snd_seq_event_t *event)
{
    DEBUG_DSSI("fsd %d: fsd_handle_event called with event type %d\n", instance->channel, event->type);

    switch (event->type) {
      case SND_SEQ_EVENT_NOTEOFF:
        fluid_synth_noteoff(fsd_synth.fluid_synth, instance->channel,
                            event->data.note.note);
        break;
      case SND_SEQ_EVENT_NOTEON:
        if (event->data.note.velocity > 0)
            fluid_synth_noteon(fsd_synth.fluid_synth, instance->channel,
                               event->data.note.note, event->data.note.velocity);
        else
            fluid_synth_noteoff(fsd_synth.fluid_synth, instance->channel,
                                event->data.note.note);
        break;
      case SND_SEQ_EVENT_KEYPRESS:
        /* FluidSynth does not implement */
        /* fluid_synth_key_pressure(fsd_synth.fluid_synth, instance->channel, event->data.note.note, event->data.note.velocity); */
        break;
      case SND_SEQ_EVENT_CONTROLLER:
        fluid_synth_cc(fsd_synth.fluid_synth, instance->channel,
                       event->data.control.param, event->data.control.value);
        break;
      case SND_SEQ_EVENT_CHANPRESS:
        /* FluidSynth does not implement */
        /* fluid_synth_channel_pressure(fsd_synth.fluid_synth, instance->channel, event->data.control.value); */
        break;
      case SND_SEQ_EVENT_PITCHBEND:
        /* ALSA pitch bend is -8192 - 8191, FluidSynth wants 0 - 16383 */
        fluid_synth_pitch_bend(fsd_synth.fluid_synth, instance->channel,
                               event->data.control.value + 8192);
        break;
      case SND_SEQ_EVENT_SYSEX:	{
        /* sysex support (needed for MTS tuning messages) */
        /* contributed by Albert Graef */
        unsigned char *data = (unsigned char*)event->data.ext.ptr;
        unsigned int len = event->data.ext.len;
#if 0   /* enable for debugging output */
        unsigned int i;
        fprintf(stderr, "SYSEX:");
        for (i = 0; i < len; ++i)
          fprintf(stderr, " %02x", data[i]);
        fprintf(stderr, "\n");
#endif
        /* fluid_synth_sysex() expects the message without the f0 f7 bytes. */
        fluid_synth_sysex(fsd_synth.fluid_synth, (char *)data+1, len-2,
                          NULL, NULL, NULL, 0);
        break;
      }
      /* SND_SEQ_EVENT_PGMCHANGE - shouldn't happen */
      /* SND_SEQ_EVENT_CONTROL14? */
      /* SND_SEQ_EVENT_NONREGPARAM? */
      /* SND_SEQ_EVENT_REGPARAM? */
      default:
        break;
    }
}

/*
 * fsd_run_multiple_synths
 *
 * implements DSSI (*run_multiple_synths)()
 */
static void
fsd_run_multiple_synths(unsigned long instance_count, LADSPA_Handle *handles,
                        unsigned long sample_count, snd_seq_event_t **events,
                        unsigned long *event_count)
{
    /* FluidSynth renders everything in blocks of FLUID_BUFSIZE (64) samples,
     * while DSSI plugins are expected to handle any sample count given them.
     * While we can't have sample-accurate event rendering here (because of the
     * FLUID_BUFSIZE blocking), we can make sure the ordering of our event
     * handling is sample-accurate across all instances of this plugin. */

    fsd_instance_t **instances = (fsd_instance_t **)handles;
    unsigned long samples_done = 0;
    unsigned long event_index[instance_count];
    unsigned long this_pending_event_tick;
    unsigned long next_pending_event_tick;
    int i;

    /* Attempt the mutex, return only silence if lock fails. */
    if (fsd_mutex_trylock()) {
        for (i = 0; i < instance_count; i++) {
            memset(instances[i]->output_l, 0, sizeof(LADSPA_Data) * sample_count);
            memset(instances[i]->output_r, 0, sizeof(LADSPA_Data) * sample_count);
        }
        return;
    }

    for (i = 0; i < instance_count; i++) {
        event_index[i] = 0;
        if (instances[i]->pending_preset_change > -1) {
            fsd_handle_pending_preset_change(instances[i]);
            instances[i]->pending_preset_change = -1;
        }
    }
    /* Trash unmapped channels by default */
    for (i = 0; i < fsd_settings.channel_count; i++) {
        l_outputs[i] = fsd_synth.bit_bucket;
        r_outputs[i] = fsd_synth.bit_bucket;
    }

    /* fluid_synth_nwrite_float() works correctly in FluidSynth beginning
     * with CVS as of 2005/6/7 (and I would assume releases > 1.0.5).
     * So here we can just have it write our output buffers directly,
     * without the extra copy. */
    /* AArgh and in FluidSynth 2.0.1 it is marked as deprecated... */

    next_pending_event_tick = 0;
    while (samples_done < sample_count) {
        unsigned long burst_size;

        /* process any ready events */
        while (next_pending_event_tick <= samples_done) {
            this_pending_event_tick = next_pending_event_tick;
            next_pending_event_tick = sample_count;
            for (i = 0; i < instance_count; i++) {
                while (event_index[i] < event_count[i]
                       && events[i][event_index[i]].time.tick == this_pending_event_tick) {
                     fsd_handle_event(instances[i], &events[i][event_index[i]]);
                     event_index[i]++;
                }
                if (event_index[i] < event_count[i]
                    && events[i][event_index[i]].time.tick < next_pending_event_tick) {
                    next_pending_event_tick = events[i][event_index[i]].time.tick;
                }
            }
        }

        /* render the burst */
        burst_size = next_pending_event_tick - samples_done;
        if (burst_size > FSD_MAX_BURST_SIZE)
            burst_size = FSD_MAX_BURST_SIZE;
        
        for (i = 0; i < fsd_settings.channel_count; i++) {
            if (fsd_synth.channel_map[i]) {
                fsd_instance_t *instance = fsd_synth.channel_map[i];
                l_outputs[instance->channel] = instance->output_l + samples_done;
                r_outputs[instance->channel] = instance->output_r + samples_done;
            }
        }

        fluid_synth_nwrite_float(fsd_synth.fluid_synth, burst_size,
                                 l_outputs, r_outputs,
                                 NULL, NULL);

        samples_done += burst_size;
    }


#ifdef DEBUG_AUDIO
/* add a 'buzz' to output so there's something audible even when quiescent */
for (i = 0; i < instance_count; i++) {
*instances[i]->output_l += 0.10f;
*instances[i]->output_r += 0.10f;
}
#endif /* DEBUG_AUDIO */

    fsd_mutex_unlock();
}

// optional:
//    void (*run_multiple_synths_adding)(unsigned long     InstanceCount,
//                                       LADSPA_Handle   **Instances,
//                                       unsigned long     SampleCount,
//                                       snd_seq_event_t **Events,
//                                       unsigned long    *EventCounts);

/* ---- export ---- */

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
        return fsd_LADSPA_descriptor;
    default:
        return NULL;
    }
}

const DSSI_Descriptor *dssi_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
        return fsd_DSSI_descriptor;
    default:
        return NULL;
    }
}

#ifdef __GNUC__
__attribute__((constructor)) void init()
#else
void _init()
#endif
{
    int i;
    char **port_names;
    LADSPA_PortDescriptor *port_descriptors;
    LADSPA_PortRangeHint *port_range_hints;

    fsd_synth.instance_count = 0;
    pthread_mutex_init(&fsd_synth.mutex, NULL);
    fsd_synth.mutex_grab_failed = 0;
    fsd_synth.soundfonts = NULL;

    fsd_LADSPA_descriptor =
        (LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));
    if (fsd_LADSPA_descriptor) {
        fsd_LADSPA_descriptor->UniqueID = 2182;
        fsd_LADSPA_descriptor->Label = "FluidSynth-DSSI";
        fsd_LADSPA_descriptor->Properties = 0;
        fsd_LADSPA_descriptor->Name = "FluidSynth DSSI plugin";
        fsd_LADSPA_descriptor->Maker = "Sean Bolton <musound AT jps DOT net>";
        fsd_LADSPA_descriptor->Copyright = "(c)2005 GNU General Public License version 2 or later";
        fsd_LADSPA_descriptor->PortCount = FSD_PORTS_COUNT;

        port_descriptors = (LADSPA_PortDescriptor *)
                                calloc(fsd_LADSPA_descriptor->PortCount, sizeof
                                                (LADSPA_PortDescriptor));
        fsd_LADSPA_descriptor->PortDescriptors =
            (const LADSPA_PortDescriptor *) port_descriptors;

        port_range_hints = (LADSPA_PortRangeHint *)
                                calloc(fsd_LADSPA_descriptor->PortCount, sizeof
                                                (LADSPA_PortRangeHint));
        fsd_LADSPA_descriptor->PortRangeHints =
            (const LADSPA_PortRangeHint *) port_range_hints;

        port_names = (char **) calloc(fsd_LADSPA_descriptor->PortCount, sizeof(char *));
        fsd_LADSPA_descriptor->PortNames = (const char **) port_names;

        for (i = 0; i < FSD_PORTS_COUNT; i++) {
            port_descriptors[i] = fsd_port_description[i].port_descriptor;
            port_names[i]       = fsd_port_description[i].name;
            port_range_hints[i].HintDescriptor = fsd_port_description[i].hint_descriptor;
            port_range_hints[i].LowerBound     = fsd_port_description[i].lower_bound;
            port_range_hints[i].UpperBound     = fsd_port_description[i].upper_bound;
        }

        fsd_LADSPA_descriptor->instantiate = fsd_instantiate;
        fsd_LADSPA_descriptor->connect_port = fsd_connect_port;
        fsd_LADSPA_descriptor->activate = fsd_activate;
        fsd_LADSPA_descriptor->run = fsd_ladspa_run;
        fsd_LADSPA_descriptor->run_adding = NULL;
        fsd_LADSPA_descriptor->set_run_adding_gain = NULL;
        fsd_LADSPA_descriptor->deactivate = fsd_deactivate;
        fsd_LADSPA_descriptor->cleanup = fsd_cleanup;
    }

    fsd_DSSI_descriptor = (DSSI_Descriptor *) malloc(sizeof(DSSI_Descriptor));
    if (fsd_DSSI_descriptor) {
        fsd_DSSI_descriptor->DSSI_API_Version = 1;
        fsd_DSSI_descriptor->LADSPA_Plugin = fsd_LADSPA_descriptor;
        fsd_DSSI_descriptor->configure = fsd_configure;
        fsd_DSSI_descriptor->get_program = fsd_get_program;
        fsd_DSSI_descriptor->select_program = fsd_select_program;
        fsd_DSSI_descriptor->get_midi_controller_for_port = fsd_get_midi_controller;
        fsd_DSSI_descriptor->run_synth = NULL;
        fsd_DSSI_descriptor->run_synth_adding = NULL;
        fsd_DSSI_descriptor->run_multiple_synths = fsd_run_multiple_synths;
        fsd_DSSI_descriptor->run_multiple_synths_adding = NULL;
    }
}

#ifdef __GNUC__
__attribute__((destructor)) void fini()
#else
void _fini()
#endif
{
    if (fsd_LADSPA_descriptor) {
        free((LADSPA_PortDescriptor *) fsd_LADSPA_descriptor->PortDescriptors);
        free((char **) fsd_LADSPA_descriptor->PortNames);
        free((LADSPA_PortRangeHint *) fsd_LADSPA_descriptor->PortRangeHints);
        free(fsd_LADSPA_descriptor);
    }
    if (fsd_DSSI_descriptor) {
        free(fsd_DSSI_descriptor);
    }
}

