/* FluidSynth DSSI software synthesizer GUI
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Portions of this file may have come from FluidSynth, copyright
 * (C) 2003 Peter Hanappe and others.
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

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>
#include <ladspa.h>
#include <lo/lo.h>

#include <fcntl.h>  /* -FIX- just for lo_server_set_nonblocking() */

#include "FluidSynth-DSSI_gtk.h"

/* ==== global variables ==== */

lo_address osc_host_address;
char *     osc_configure_path;
char *     osc_control_path;
char *     osc_exiting_path;
char *     osc_hide_path;
char *     osc_midi_path;
char *     osc_program_path;
char *     osc_quit_path;
char *     osc_show_path;
char *     osc_update_path;

SFData *      soundfont_data = NULL;
char          soundfont_filename[PATH_MAX];
unsigned long preset_count = 0;
SFPreset **   presets_by_row = NULL;

unsigned char test_note_noteon_key = 60;
unsigned char test_note_noteoff_key;
unsigned char test_note_velocity = 96;

GtkWidget *main_window;
GtkWidget *soundfont_label;
GtkWidget *preset_clist;
GtkWidget *file_selection;
GtkWidget *notice_window;
GtkWidget *notice_label_1;
GtkWidget *notice_label_2;

int internal_gui_update_only = 0;
int host_requested_quit = 0;

/* ==== a couple things that liblo will soon have ==== */

int
lo_server_get_socket_fd(lo_server *server)
{
    return *(int *)server;  /* !FIX! ack. _will_ break, but saves us from needing lo_types_internal.h here.*/
}

int
lo_server_set_nonblocking(lo_server *server)
{
    /* make the server socket non-blocking */
    return fcntl(lo_server_get_socket_fd(server), F_SETFL, O_NONBLOCK);
}

/* ==== OSC handling ==== */

static char *
osc_build_path(char *base_path, char *method)
{
    char buffer[256];
    char *full_path;

    snprintf(buffer, 256, "%s%s", base_path, method);
    if (!(full_path = strdup(buffer))) {
        DEBUG_DSSI("fsd-gui: out of memory!\n");
        exit(1);
    }
    return strdup(buffer);
}

static void
osc_error(int num, const char *msg, const char *path)
{
    DEBUG_DSSI("fsd-gui error: liblo server error %d in path \"%s\": %s\n",
            num, (path ? path : "(null)"), msg);
}

int
osc_debug_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    int i;

    DEBUG_DSSI("fsd-gui warning: unhandled OSC message to <%s>:\n", path);

    for (i = 0; i < argc; ++i) {
        fprintf(stderr, "arg %d: type '%c': ", i, types[i]);
fflush(stderr);
        lo_arg_pp((lo_type)types[i], argv[i]);  /* -FIX- Ack, mixing stderr and stdout... */
        fprintf(stdout, "\n");
fflush(stdout);
    }

    return 1;  /* try any other handlers */
}

int
osc_action_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    if (!strcmp(user_data, "show")) {

        /* DEBUG_DSSI("fsd-gui osc_action_handler: received 'show' message\n"); */
        if (!GTK_WIDGET_MAPPED(main_window))
            gtk_widget_show(main_window);
        else
            gdk_window_raise(main_window->window);

    } else if (!strcmp(user_data, "hide")) {

        /* DEBUG_DSSI("fsd-gui osc_action_handler: received 'hide' message\n"); */
        gtk_widget_hide(main_window);

    } else if (!strcmp(user_data, "quit")) {

        /* DEBUG_DSSI("fsd-gui osc_action_handler: received 'quit' message\n"); */
        host_requested_quit = 1;
        gtk_main_quit();

    } else {

        return osc_debug_handler(path, types, argv, argc, msg, user_data);

    }
    return 0;
}

int
osc_configure_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    if (argc < 2) {
        DEBUG_DSSI("fsd-gui error: too few arguments to osc_configure_handler\n");
        return 1;
    }

    if (!strcmp(&argv[0]->s, "load")) {

        int result;

        internal_gui_update_only = 1;
        result = load_soundfont(&argv[1]->s);
        internal_gui_update_only = 0;

        if (!result) {
            DEBUG_DSSI("fsd-gui osc_configure_handler: load_soundfont() failed!\n");
            gtk_label_set_text (GTK_LABEL (notice_label_1), "Unable to load the soundfont requested by the host!");
            gtk_label_set_text (GTK_LABEL (notice_label_2), &argv[1]->s);
            gtk_widget_show(notice_window);
        }

        return 0;

    } else {

        return osc_debug_handler(path, types, argv, argc, msg, user_data);

    }
}

int
osc_control_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    /* though for now, we have no controls to handle.... */
    int port;
    float value;

    if (argc < 2) {
        DEBUG_DSSI("fsd-gui error: too few arguments to osc_control_handler\n");
        return 1;
    }

    port = argv[0]->i;
    value = argv[1]->f;

    DEBUG_DSSI("fsd-gui osc_control_handler: (bogus) control %d now %f\n", port, value);

    /* update_voice_widget(port, value); */

    return 0;
}

int
osc_program_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    int bank, program;

    if (argc < 2) {
        DEBUG_DSSI("fsd-gui error: too few arguments to osc_program_handler\n");
        return 1;
    }

    bank = argv[0]->i;
    program = argv[1]->i;

    if (bank < 0 || bank > 16383 || program < 0 || program > 127) {
        DEBUG_DSSI("fsd-gui: out-of-range program select (bank %d, program %d)\n", bank, program);
        return 0;
    }

    DEBUG_DSSI("fsd-gui osc_program_handler: received program change, bank %d, program %d\n", bank, program);

    update_from_program_select(bank, program);

    return 0;
}

void
osc_data_on_socket_callback(gpointer data, gint source,
                            GdkInputCondition condition)
{
    lo_server *server = (lo_server *)data;

    lo_server_recv(server);                                                 
}

/* ==== GTK+ widget callbacks ==== */

gint
on_delete_event_wrapper( GtkWidget *widget, GdkEvent *event, gpointer data )
{
    void (*handler)(GtkWidget *, gpointer) = (void (*)(GtkWidget *, gpointer))data;

    /* call our 'close', 'dismiss' or 'cancel' callback (which must not need the user data) */
    (*handler)(widget, NULL);

    /* tell GTK+ to NOT emit 'destroy' */
    return TRUE;
}

void
on_select_soundfont_button_press(GtkWidget *widget, gpointer data)
{
    gtk_widget_show(file_selection);
}

int
load_soundfont(char *filename)
{
    SFData *sfdata;

    if ((sfdata = sfload_file(filename))) {
        /* close the soundfont, since we have all we need here */
        if (sfdata->sffd) {
            fclose (sfdata->sffd);
            sfdata->sffd = NULL;
        }

        /* free the old soundfont data, if any */
        if (soundfont_data)
            sfont_free_data(soundfont_data);

        soundfont_data = sfdata;
        strncpy(soundfont_filename, filename, PATH_MAX);

        gtk_label_set_text (GTK_LABEL (soundfont_label), filename);

        rebuild_preset_clist(sfdata);

        if (!internal_gui_update_only) {
            lo_send(osc_host_address, osc_configure_path, "ss", "load",
                    filename);
        }

        return 1;
    }
    return 0;
}

void
on_file_selection_ok( GtkWidget *widget, gpointer data )
{
    gchar *filename = gtk_file_selection_get_filename(
                          GTK_FILE_SELECTION(file_selection));

    gtk_widget_hide(file_selection);

    DEBUG_DSSI("fsd-gui on_file_selection_ok: file '%s' selected\n",
               filename);

    if (!load_soundfont(filename)) {

        DEBUG_DSSI("fsd-gui on_file_selection_ok: load_soundfont() failed!\n");
        gtk_label_set_text (GTK_LABEL (notice_label_1), "Unable to load the selected soundfont!");
        gtk_label_set_text (GTK_LABEL (notice_label_2), filename);
        gtk_widget_show(notice_window);

    }
}

void
on_file_selection_cancel( GtkWidget *widget, gpointer data )
{
    /* DEBUG_DSSI("fsd-gui: on_file_selection_cancel called\n"); */
    gtk_widget_hide(file_selection);
}

void
on_preset_selection(GtkWidget      *clist,
                    gint            row,
                    gint            column,
                    GdkEventButton *event,
                    gpointer        data )
{
    if (internal_gui_update_only) {
        /* DEBUG_DSSI("fsd-gui on_preset_selection: skipping further action\n"); */
        return;
    }

    if (row < preset_count) {

        DEBUG_DSSI("fsd-gui on_preset_selection: preset %d selected => bank %d program %d\n",
                   row, presets_by_row[row]->bank, presets_by_row[row]->prenum);

        lo_send(osc_host_address, osc_program_path, "ii",
                presets_by_row[row]->bank, presets_by_row[row]->prenum);

    } else {

        DEBUG_DSSI("fsd-gui on_preset_selection: out-of-range preset %d selected\n", row);

    }
}

void
on_test_note_slider_change(GtkWidget *widget, gpointer data)
{
    unsigned char value = lrintf(GTK_ADJUSTMENT(widget)->value);

    if ((int)data == 0) {  /* key */

        test_note_noteon_key = value;
        DEBUG_DSSI("fsd-gui on_test_note_slider_change: new test note key %d\n", test_note_noteon_key);

    } else {  /* velocity */

        test_note_velocity = value;
        DEBUG_DSSI("fsd-gui on_test_note_slider_change: new test note velocity %d\n", test_note_velocity);

    }
}

void
on_test_note_button_press(GtkWidget *widget, gpointer data)
{
    unsigned char midi[4];

    if ((int)data) {  /* button pressed */

        midi[0] = 0;
        midi[1] = 0x90;
        midi[2] = test_note_noteon_key;
        midi[3] = test_note_velocity;
        lo_send(osc_host_address, osc_midi_path, "m", midi);
        test_note_noteoff_key = test_note_noteon_key;

    } else { /* button released */

        midi[0] = 0;
        midi[1] = 0x80;
        midi[2] = test_note_noteoff_key;
        midi[3] = 0x40;
        lo_send(osc_host_address, osc_midi_path, "m", midi);

    }
}

void
on_notice_dismiss( GtkWidget *widget, gpointer data )
{
    gtk_widget_hide(notice_window);
}

void
update_from_program_select(int bank, int program)
{
    int i;

    /* find the preset */
    for (i = 0; i < preset_count; i++) {
        if (presets_by_row[i]->bank == bank &&
            presets_by_row[i]->prenum == program) {
            break;
        }
    }

    if (i < preset_count) {

        internal_gui_update_only = 1;
        gtk_clist_select_row (GTK_CLIST(preset_clist), i, 0);
        internal_gui_update_only = 0;

    } else {  /* not found */

        gtk_clist_unselect_all (GTK_CLIST(preset_clist));

    }
}

void
rebuild_preset_clist(SFData *sfdata)
{
    fluid_list_t *p;
    SFPreset *sfpreset;
    char bank[6], program[4], name[31];
    char *data[3] = { bank, program, name };
    int i;

    /* DEBUG_DSSI("fsd-gui: rebuild_preset_clist called\n"); */

    gtk_clist_freeze(GTK_CLIST(preset_clist));
    gtk_clist_clear(GTK_CLIST(preset_clist));

    /* count presets */
    preset_count = 0;
    p = sfdata->preset;
    while (p != NULL) {
        preset_count++;
        p = fluid_list_next(p);
    }

    /* build presets_by_row */
    if (presets_by_row)
        free(presets_by_row);
    presets_by_row = (SFPreset **)malloc(preset_count * sizeof(SFPreset *));
    if (!presets_by_row) {
        preset_count = 0;
        gtk_clist_thaw(GTK_CLIST(preset_clist));
        return;
    }
    i = 0;
    p = sfdata->preset;
    while (p != NULL) {
        presets_by_row[i] = (SFPreset *)p->data;
        i++;
        p = fluid_list_next(p);
    }

    /* build clist */
    for (i = 0; i < preset_count; i++) {
        sfpreset = presets_by_row[i];
        snprintf(bank, 6, "%d", sfpreset->bank);
        snprintf(program, 4, "%d", sfpreset->prenum);
        if (sfpreset->name && strlen(sfpreset->name) > 0) {
            strncpy(name, sfpreset->name, 31);
        } else {
            snprintf(name, 31, "preset %d:%d", sfpreset->bank, sfpreset->prenum);
        }
        gtk_clist_append(GTK_CLIST(preset_clist), data);
    }

    gtk_clist_thaw(GTK_CLIST(preset_clist));
}

/* ==== GTK+ widget creation ==== */

GtkWidget*
create_main_window (const char *tag)
{
  GtkWidget *vbox4;
  GtkWidget *frame2;
  GtkWidget *frame4;
  GtkWidget *scrolledwindow1;
  GtkWidget *label11;
  GtkWidget *label12;
  GtkWidget *label13;
  GtkWidget *frame5;
  GtkWidget *table6;
  GtkWidget *label6;
  GtkWidget *label7;
  GtkWidget *key_scale;
  GtkWidget *velocity_scale;
  GtkWidget *hbox1;
  GtkWidget *select_soundfont_button;
  GtkWidget *test_note_button;

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (main_window), "main_window", main_window);
  gtk_window_set_title (GTK_WINDOW (main_window), tag);

  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox4);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "vbox4", vbox4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox4);
  gtk_container_add (GTK_CONTAINER (main_window), vbox4);

  frame2 = gtk_frame_new ("Soundfont");
  gtk_widget_ref (frame2);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame2", frame2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox4), frame2, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 5);

  soundfont_label = gtk_label_new ("<none loaded>");
  gtk_widget_ref (soundfont_label);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "soundfont_label", soundfont_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (soundfont_label);
  gtk_container_add (GTK_CONTAINER (frame2), soundfont_label);
  gtk_misc_set_padding (GTK_MISC (soundfont_label), 6, 0);

  frame4 = gtk_frame_new ("Preset");
  gtk_widget_ref (frame4);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame4", frame4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame4);
  gtk_box_pack_start (GTK_BOX (vbox4), frame4, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame4), 5);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_container_add (GTK_CONTAINER (frame4), scrolledwindow1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  preset_clist = gtk_clist_new (3);
  gtk_widget_ref (preset_clist);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "preset_clist", preset_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (preset_clist);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), preset_clist);
  gtk_widget_set_usize (preset_clist, 300, 100);
  gtk_clist_set_column_width (GTK_CLIST (preset_clist), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (preset_clist), 1, 80);
  gtk_clist_set_column_width (GTK_CLIST (preset_clist), 2, 80);
  gtk_clist_column_titles_show (GTK_CLIST (preset_clist));

  label11 = gtk_label_new ("Bank");
  gtk_widget_ref (label11);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label11", label11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label11);
  gtk_clist_set_column_widget (GTK_CLIST (preset_clist), 0, label11);

  label12 = gtk_label_new ("Program");
  gtk_widget_ref (label12);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label12", label12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label12);
  gtk_clist_set_column_widget (GTK_CLIST (preset_clist), 1, label12);

  label13 = gtk_label_new ("Name");
  gtk_widget_ref (label13);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label13", label13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label13);
  gtk_clist_set_column_widget (GTK_CLIST (preset_clist), 2, label13);

  frame5 = gtk_frame_new ("Test Note");
  gtk_widget_ref (frame5);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "frame5", frame5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame5);
  gtk_box_pack_start (GTK_BOX (vbox4), frame5, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame5), 5);

  table6 = gtk_table_new (2, 2, FALSE);
  gtk_widget_ref (table6);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "table6", table6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table6);
  gtk_container_add (GTK_CONTAINER (frame5), table6);
  gtk_container_set_border_width (GTK_CONTAINER (table6), 4);

  label6 = gtk_label_new ("key");
  gtk_widget_ref (label6);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label6", label6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table6), label6, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label6), 5, 0);

  label7 = gtk_label_new ("velocity");
  gtk_widget_ref (label7);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "label7", label7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label7);
  gtk_table_attach (GTK_TABLE (table6), label7, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label7), 5, 0);

  key_scale = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (60, 12, 132, 1, 12, 12)));
  gtk_widget_ref (key_scale);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "key_scale", key_scale,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (key_scale);
  gtk_table_attach (GTK_TABLE (table6), key_scale, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (key_scale), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (key_scale), 0);

  velocity_scale = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (96, 1, 137, 1, 10, 10)));
  gtk_widget_ref (velocity_scale);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "velocity_scale", velocity_scale,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (velocity_scale);
  gtk_table_attach (GTK_TABLE (table6), velocity_scale, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (velocity_scale), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (velocity_scale), 0);

  hbox1 = gtk_hbox_new (FALSE, 40);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox1, FALSE, FALSE, 0);

  select_soundfont_button = gtk_button_new_with_label ("Select Soundfont");
  gtk_widget_ref (select_soundfont_button);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "select_soundfont_button", select_soundfont_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (select_soundfont_button);
  gtk_box_pack_start (GTK_BOX (hbox1), select_soundfont_button, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (select_soundfont_button), 8);

  test_note_button = gtk_button_new_with_label ("Send Test Note");
  gtk_widget_ref (test_note_button);
  gtk_object_set_data_full (GTK_OBJECT (main_window), "test_note_button", test_note_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (test_note_button);
  gtk_box_pack_end (GTK_BOX (hbox1), test_note_button, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (test_note_button), 8);

    /* connect main window */
    gtk_signal_connect(GTK_OBJECT(main_window), "destroy",
                       GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (main_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)gtk_main_quit);

    /* connect select soundfont button */
    gtk_signal_connect (GTK_OBJECT (select_soundfont_button), "clicked",
                        GTK_SIGNAL_FUNC (on_select_soundfont_button_press),
                        NULL);

    /* connect preset clist */
    gtk_signal_connect(GTK_OBJECT(preset_clist), "select_row",
                       GTK_SIGNAL_FUNC(on_preset_selection),
                       NULL);

    /* connect test note widgets */
    gtk_signal_connect (GTK_OBJECT (gtk_range_get_adjustment (GTK_RANGE (key_scale))),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        (gpointer)0);
    gtk_signal_connect (GTK_OBJECT (gtk_range_get_adjustment (GTK_RANGE (velocity_scale))),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        (gpointer)1);
    gtk_signal_connect (GTK_OBJECT (test_note_button), "pressed",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        (gpointer)1);
    gtk_signal_connect (GTK_OBJECT (test_note_button), "released",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        (gpointer)0);

  return main_window;
}

GtkWidget*
create_file_selection (const char *tag)
{
  char      *title;
  GtkWidget *ok_button1;
  GtkWidget *cancel_button1;

    title = (char *)malloc(strlen(tag) + 20);
    sprintf(title, "%s - Select Soundfont", tag);
    file_selection = gtk_file_selection_new (title);
    free(title);
  gtk_object_set_data (GTK_OBJECT (file_selection), "file_selection", file_selection);
  gtk_container_set_border_width (GTK_CONTAINER (file_selection), 10);

  ok_button1 = GTK_FILE_SELECTION (file_selection)->ok_button;
  gtk_object_set_data (GTK_OBJECT (file_selection), "ok_button1", ok_button1);
  gtk_widget_show (ok_button1);
  GTK_WIDGET_SET_FLAGS (ok_button1, GTK_CAN_DEFAULT);

  cancel_button1 = GTK_FILE_SELECTION (file_selection)->cancel_button;
  gtk_object_set_data (GTK_OBJECT (file_selection), "cancel_button1", cancel_button1);
  gtk_widget_show (cancel_button1);
  GTK_WIDGET_SET_FLAGS (cancel_button1, GTK_CAN_DEFAULT);

    gtk_signal_connect (GTK_OBJECT (file_selection), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (file_selection), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_file_selection_cancel);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selection)->ok_button),
                        "clicked", (GtkSignalFunc)on_file_selection_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selection)->cancel_button),
                        "clicked", (GtkSignalFunc)on_file_selection_cancel,
                        NULL);

  return file_selection;
}

GtkWidget*
create_notice_window (const char *tag)
{
  char      *title;
  GtkWidget *vbox3;
  GtkWidget *hbox1;
  GtkWidget *notice_dismiss;

    notice_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (notice_window), "notice_window", notice_window);
    title = (char *)malloc(strlen(tag) + 8);
    sprintf(title, "%s Notice", tag);
    gtk_window_set_title (GTK_WINDOW (notice_window), title);
    free(title);
    gtk_window_set_position (GTK_WINDOW (notice_window), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal (GTK_WINDOW (notice_window), TRUE);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox3);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "vbox3", vbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (notice_window), vbox3);

  notice_label_1 = gtk_label_new ("Some message\ngoes here");
  gtk_widget_ref (notice_label_1);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_label_1", notice_label_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notice_label_1);
  gtk_box_pack_start (GTK_BOX (vbox3), notice_label_1, TRUE, TRUE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (notice_label_1), TRUE);
  gtk_misc_set_padding (GTK_MISC (notice_label_1), 10, 5);

  notice_label_2 = gtk_label_new ("more text\ngoes here");
  gtk_widget_ref (notice_label_2);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_label_2", notice_label_2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notice_label_2);
  gtk_box_pack_start (GTK_BOX (vbox3), notice_label_2, FALSE, FALSE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (notice_label_2), TRUE);
  gtk_misc_set_padding (GTK_MISC (notice_label_2), 10, 5);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox1, FALSE, FALSE, 0);

  notice_dismiss = gtk_button_new_with_label ("Dismiss");
  gtk_widget_ref (notice_dismiss);
  gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_dismiss", notice_dismiss,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notice_dismiss);
  gtk_box_pack_start (GTK_BOX (hbox1), notice_dismiss, TRUE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (notice_dismiss), 7);

    gtk_signal_connect (GTK_OBJECT (notice_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (notice_window), "delete_event",
                        GTK_SIGNAL_FUNC (on_delete_event_wrapper),
                        (gpointer)on_notice_dismiss);
    gtk_signal_connect (GTK_OBJECT (notice_dismiss), "clicked",
                        GTK_SIGNAL_FUNC (on_notice_dismiss),
                        NULL);
    
    return notice_window;
}

void
create_windows(const char *instance_tag)
{
    char tag[50];

    /* build a nice identifier string for the window titles */
    if (strlen(instance_tag) == 0) {
        strcpy(tag, "FluidSynth-DSSI");
    } else if (strstr(instance_tag, "FluidSynth-DSSI") ||
               strstr(instance_tag, "fluidsynth-dssi")) {
        if (strlen(instance_tag) > 49) {
            snprintf(tag, 50, "...%s", instance_tag + strlen(instance_tag) - 46); /* hope the unique info is at the end */
        } else {
            strcpy(tag, instance_tag);
        }
    } else {
        if (strlen(instance_tag) > 33) {
            snprintf(tag, 50, "FluidSynth-DSSI ...%s", instance_tag + strlen(instance_tag) - 30);
        } else {
            snprintf(tag, 50, "FluidSynth-DSSI %s", instance_tag);
        }
    }

    create_main_window (tag);
    create_file_selection (tag);
    create_notice_window (tag);
}

/* ==== main ==== */

int
main (int argc, char *argv[])
{
    char *host, *port, *path, *tmp_url, *self_url;
    lo_server osc_server;
    gint osc_server_socket_tag;

    fprintf(stderr, "fsd-gui starting (pid %d)...\n", getpid());

    gtk_set_locale ();
    gtk_init (&argc, &argv);

    if (argc != 5) {
        fprintf(stderr, "usage: %s <osc url> <plugin dllname> <plugin label> <user-friendly id>\n", argv[0]);
        exit(1);
    }

    /* set up OSC support */
    host = lo_url_get_hostname(argv[1]);
    port = lo_url_get_port(argv[1]);
    path = lo_url_get_path(argv[1]);
    osc_host_address = lo_address_new(host, port);
    osc_configure_path = osc_build_path(path, "/configure");
    osc_control_path   = osc_build_path(path, "/control");
    osc_exiting_path   = osc_build_path(path, "/exiting");
    osc_hide_path      = osc_build_path(path, "/hide");
    osc_midi_path      = osc_build_path(path, "/midi");
    osc_program_path   = osc_build_path(path, "/program");
    osc_quit_path      = osc_build_path(path, "/quit");
    osc_show_path      = osc_build_path(path, "/show");
    osc_update_path    = osc_build_path(path, "/update");

    osc_server = lo_server_new(NULL, osc_error);
    if (lo_server_set_nonblocking(osc_server)) {
        DEBUG_DSSI("fsd-gui error: could not set make OSC server non-blocking!\n");
        /* bumpy ride! */
    }
    lo_server_add_method(osc_server, osc_configure_path, "ss", osc_configure_handler, NULL);
    lo_server_add_method(osc_server, osc_control_path, "if", osc_control_handler, NULL);
    lo_server_add_method(osc_server, osc_hide_path, "", osc_action_handler, "hide");
    lo_server_add_method(osc_server, osc_program_path, "ii", osc_program_handler, NULL);
    lo_server_add_method(osc_server, osc_quit_path, "", osc_action_handler, "quit");
    lo_server_add_method(osc_server, osc_show_path, "", osc_action_handler, "show");
    lo_server_add_method(osc_server, NULL, NULL, osc_debug_handler, NULL);

    tmp_url = lo_server_get_url(osc_server);
    self_url = osc_build_path(tmp_url, (strlen(path) > 1 ? path + 1 : path));
    free(tmp_url);

    /* set up GTK+ */
    create_windows(argv[4]);

    /* add OSC server socket to GTK+'s watched I/O */
    osc_server_socket_tag = gdk_input_add(lo_server_get_socket_fd(osc_server),
                                          GDK_INPUT_READ,
                                          osc_data_on_socket_callback,
                                          osc_server);

    /* send our update request */
    lo_send(osc_host_address, osc_update_path, "s", self_url);

    /* let GTK+ take it from here */
    gtk_main();

    /* clean up and exit */
    DEBUG_DSSI("fsd-gui: yep, we got to the cleanup!\n");

    /* GTK+ cleanup */
    gdk_input_remove(osc_server_socket_tag);

    /* say bye-bye */
    if (!host_requested_quit) {
        lo_send(osc_host_address, osc_exiting_path, "");
    }

    /* free soundfont data, if any */
    if (soundfont_data)
        sfont_free_data(soundfont_data);
    if (presets_by_row)
        free(presets_by_row);

    /* clean up OSC support */
    lo_server_free(osc_server);
    free(host);
    free(port);
    free(path);
    free(osc_configure_path);
    free(osc_control_path);
    free(osc_exiting_path);
    free(osc_hide_path);
    free(osc_midi_path);
    free(osc_program_path);
    free(osc_quit_path);
    free(osc_show_path);
    free(osc_update_path);

    return 0;
}

