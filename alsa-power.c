/*
 * Copyright 2009, Intel Corporation
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
#include <assert.h>

#include "powertop.h"

struct device_data;

struct device_data {
	struct device_data *next;
	char pathname[4096];
	char human_name[4096];
	uint64_t power_on, power_off;
	uint64_t previous_power_on, previous_power_off;
};


static struct device_data *devices;

static void cachunk_data(void)
{
	struct device_data *ptr;
	ptr = devices;
	while (ptr) {
		ptr->previous_power_on = ptr->power_on;
		ptr->previous_power_off = ptr->power_off;
		ptr = ptr->next;
	}
}

static char *long_name(char *path, char *shortname)
{
	static char line[4096];
	char filename[4096], temp[4096], *c;
	FILE *file;
	sprintf(line, "%s ", shortname);

	sprintf(filename, "%s/vendor_name", path);
	file = fopen(filename, "r");
	if (file) {
		if (fgets(temp, 4096, file))
			strcat(line, temp);
		fclose(file);
	}

	sprintf(filename, "%s/chip_name", path);
	file = fopen(filename, "r");
	if (file) {
		if (fgets(temp, 4096, file))
			strcat(line, temp);
		fclose(file);
	}

	while ((c = strchr(line, '\n')))
		*c = ' ';
	return line;
}
static void update_alsa_device(char *path, char *shortname)
{
	struct device_data *ptr;
	FILE *file;
	char fullpath[4096];
	char name[4096];
	ptr = devices;

	sprintf(fullpath, "%s/power_off_acct", path);

	if (access(fullpath, R_OK))
		return;

	while (ptr) {
		if (strcmp(ptr->pathname, path)==0) {
			file = fopen(fullpath, "r");
			if (!file)
				return;
			fgets(name, 4096, file);
			ptr->power_off = strtoull(name, NULL, 10);
			fclose(file);
			sprintf(fullpath, "%s/power_on_acct", path);
			file = fopen(fullpath, "r");
			if (!file)
				return;
			fgets(name, 4096, file);
			ptr->power_on = strtoull(name, NULL, 10);
			fclose(file);

			return;
		}
		ptr = ptr->next;
	}
	/* no luck, new one */
	ptr = malloc(sizeof(struct device_data));
	assert(ptr!=0);
	memset(ptr, 0, sizeof(struct device_data));
	ptr->next = devices;
	devices = ptr;
	strcpy(ptr->pathname, path);
	strcpy(ptr->human_name, long_name(path, shortname));
}

void do_alsa_stats(void)
{
	DIR *dir;
	struct dirent *dirent;
	char pathname[PATH_MAX];

	dir = opendir("/sys/class/sound/card0");
	if (!dir)
		return;
		
	cachunk_data();
	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.')
			continue;

		sprintf(pathname, "/sys/class/sound/card0/%s", dirent->d_name);
		update_alsa_device(pathname, dirent->d_name);
	}

	closedir(dir);
}


void display_alsa_activity(void)
{
	struct device_data *dev;
	printf("\n");
	printf("%s\n", _("Recent audio activity statistics"));
	printf("%s\n", _("Active  Device name"));
	dev = devices;
	while (dev) {
		printf("%5.1f%%\t%s\n", 100.0*(dev->power_on - dev->previous_power_on) / 
			(0.00001 + dev->power_on + dev->power_off - dev->previous_power_on - dev->previous_power_off), 
			dev->human_name);
		dev = dev->next;
	}

}

void alsa_activity_hint(void)
{
	int total_active = 0;
	int pick;
	struct device_data *dev;
	dev = devices;
	while (dev) {
		if (dev->power_on-1 > dev->previous_power_on)
			total_active++;
		dev = dev->next;
	}
	if (!total_active)
		return;

	pick = rand() % total_active;
	total_active = 0;
	dev = devices;
	while (dev) {
		if (dev->power_on-1 > dev->previous_power_on) {
			if (total_active == pick) {
				char alsa_hint[8000];
				sprintf(alsa_hint, _("An audio device is active %4.1f%% of the time:\n%s"),
				 100.0*(dev->power_on - dev->previous_power_on) / 
				(0.00001 + dev->power_on + dev->power_off - dev->previous_power_on - dev->previous_power_off),
				dev->human_name);
				add_suggestion(alsa_hint,
				1, 'A', _(" A - Turn HD audio powersave on "), hda_power_on);
			}
			total_active++;
		}
		dev = dev->next;
	}

}
