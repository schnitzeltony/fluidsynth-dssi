/* FluidSynth DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Chris Cannam and others.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_SF2PATH "/usr/local/share/sf2:/usr/share/sf2"

char *
fsd_locate_soundfont_file(const char *origpath, const char *projectDirectory)
{
    struct stat statbuf;
    char *sf2path, *path, *origPath, *element, *eltpath;
    const char *filename;

    if (stat(origpath, &statbuf) == 0)
        return strdup(origpath);

    filename = strrchr(origpath, '/');

    if (filename) ++filename;
    else filename = origpath;
    if (!*filename) return NULL;

    sf2path = getenv("SF2_PATH");
    if (sf2path) path = strdup(sf2path);
    else {
	char *home = getenv("HOME");
	if (!home) path = strdup(DEFAULT_SF2PATH);
	else {
	    path = (char *)malloc(strlen(DEFAULT_SF2PATH) + strlen(home) + 6);
	    sprintf(path, "%s/sf2:%s", home, DEFAULT_SF2PATH);
	}
    }

    if (projectDirectory) {
	origPath = path;
	path = (char *)malloc(strlen(origPath) + strlen(projectDirectory) + 2);
	sprintf(path, "%s:%s", projectDirectory, origPath);
	free(origPath);
    }

    origPath = path;

    while ((element = strtok(path, ":")) != 0) {

	path = 0;

	if (element[0] != '/') {
	    /* DEBUG_DSSI("fluidsynth-dssi: Ignoring relative element %s in path\n", element); */
	    continue;
	}

	eltpath = (char *)malloc(strlen(element) + strlen(filename) + 2);
	sprintf(eltpath, "%s/%s", element, filename);

	if (stat(eltpath, &statbuf) == 0) {
	    free(origPath);
	    return eltpath;
	}

	free(eltpath);
    }

    free(origPath);
    return NULL;
}

