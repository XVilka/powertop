/*
 * Copyright 2007, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * Authors:
 * 	Arjan van de Ven <arjan@linux.intel.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>

#include "powertop.h"

void activate_usb_autosuspend(void)
{
	DIR *dir;
	struct dirent *dirent;
	FILE *file;
	char filename[PATH_MAX];
	int len;
	char linkto[PATH_MAX];

	dir = opendir("/sys/bus/usb/devices");
	if (!dir)
		return;

	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.')
			continue;

		/* skip usb input devices */
		sprintf(filename, "/sys/bus/usb/devices/%s/driver", dirent->d_name);
		len = readlink(filename, linkto, sizeof(link) - 1);
		if (strstr(linkto, "usbhid"))
			continue;

		sprintf(filename, "/sys/bus/usb/devices/%s/power/level", dirent->d_name);
		file = fopen(filename, "w");
		if (!file)
			continue;
		fprintf(file, "auto\n");
		fclose(file);
	}

	closedir(dir);
}

void suggest_usb_autosuspend(void)
{
	DIR *dir;
	struct dirent *dirent;
	FILE *file;
	char filename[PATH_MAX];
	char line[1024];
	int len;
	char linkto[PATH_MAX];
	int need_hint = 0;

	memset(linkto, 0, sizeof(linkto));

	dir = opendir("/sys/bus/usb/devices");
	if (!dir)
		return;

	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.')
			continue;

		/* skip usb input devices */
		sprintf(filename, "/sys/bus/usb/devices/%s/driver", dirent->d_name);
		len = readlink(filename, linkto, sizeof(link) - 1);
		if (strstr(linkto, "usbhid"))
			continue;

		sprintf(filename, "/sys/bus/usb/devices/%s/power/level", dirent->d_name);
		file = fopen(filename, "r");
		if (!file)
			continue;
		memset(line, 0, 1024);
		if (fgets(line, 1023,file)==NULL) {
			fclose(file);
			continue;
		}
		if (strstr(line, "on"))
			need_hint = 1;

		fclose(file);


	}

	closedir(dir);

	if (need_hint) {
		add_suggestion(_("Suggestion: Enable USB autosuspend for non-input devices by pressing the U key\n"
				 ),
				45, 'U', _(" U - Enable USB suspend "), activate_usb_autosuspend);
	}
}


