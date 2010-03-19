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
	uint64_t active, partial, slumber, total;
	uint64_t previous_active, previous_partial, previous_slumber, previous_total;
};


static struct device_data *devices;

static void cachunk_data(void)
{
	struct device_data *ptr;
	ptr = devices;
	while (ptr) {
		ptr->previous_active = ptr->active;
		ptr->previous_partial = ptr->partial;
		ptr->previous_slumber = ptr->slumber;
		ptr->previous_total = ptr->total;
		ptr = ptr->next;
	}
}

static char *disk_name(char *path, char *target, char *shortname)
{

	DIR *dir;
	struct dirent *dirent;
	char pathname[PATH_MAX];

	sprintf(pathname, "%s/%s", path, target);
	dir = opendir(pathname);
	if (!dir)
		return strdup(shortname);
		
	while ((dirent = readdir(dir))) {
		char line[4096], *c;
		FILE *file;
		if (dirent->d_name[0]=='.')
			continue;

		if (!strchr(dirent->d_name, ':'))
			continue;

		sprintf(line, "%s/%s/model", pathname, dirent->d_name);
		file = fopen(line, "r");
		if (file) {
			if (fgets(line, 4096, file) == NULL)
				return strdup(shortname);
			fclose(file);
			c = strchr(line, '\n');
			if (c)
				*c = 0;
			return strdup(line);
		}
	}
	closedir(dir);
	
	return strdup(shortname);
}

static char *model_name(char *path, char *shortname)
{

	DIR *dir;
	struct dirent *dirent;
	char pathname[PATH_MAX];

	sprintf(pathname, "%s/device", path);

	dir = opendir(pathname);
	if (!dir)
		return strdup(shortname);
		
	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.')
			continue;

		if (!strchr(dirent->d_name, ':'))
			continue;
		if (!strstr(dirent->d_name, "target"))
			continue;
		return disk_name(pathname, dirent->d_name, shortname);
	}
	closedir(dir);
	
	return strdup(shortname);
}

static int first_time = 1;
static void update_ahci_device(char *path, char *shortname)
{
	struct device_data *ptr;
	FILE *file;
	char fullpath[4096];
	char name[4096], *c;
	ptr = devices;

	sprintf(fullpath, "%s/ahci_alpm_accounting", path);

	if (access(fullpath, R_OK))
		return;

	if (first_time) {
		file = fopen(fullpath, "w");
		if (file) fprintf(file, "1\n");
		if (file) fclose(file);
	}

	while (ptr) {
		if (strcmp(ptr->pathname, path)==0) {
			sprintf(fullpath, "%s/ahci_alpm_active", path);
			file = fopen(fullpath, "r");
			if (!file)
				return;
			fgets(name, 4096, file);
			ptr->active = strtoull(name, NULL, 10);
			fclose(file);

			sprintf(fullpath, "%s/ahci_alpm_partial", path);
			file = fopen(fullpath, "r");
			if (!file)
				return;
			fgets(name, 4096, file);
			ptr->partial = strtoull(name, NULL, 10);
			fclose(file);

			sprintf(fullpath, "%s/ahci_alpm_slumber", path);
			file = fopen(fullpath, "r");
			if (!file)
				return;
			fgets(name, 4096, file);
			ptr->slumber = strtoull(name, NULL, 10);
			fclose(file);

			ptr->total = ptr->active + ptr->partial + ptr->slumber;
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
	c = model_name(path, shortname);
	strcpy(ptr->human_name, c);
	free(c);
}

void do_ahci_stats(void)
{
	DIR *dir;
	struct dirent *dirent;
	char pathname[PATH_MAX];

	dir = opendir("/sys/class/scsi_host");
	if (!dir)
		return;
		
	cachunk_data();
	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.')
			continue;

		sprintf(pathname, "/sys/class/scsi_host/%s", dirent->d_name);
		update_ahci_device(pathname, dirent->d_name);
	}

	closedir(dir);
	first_time = 0;
}


void display_ahci_activity(void)
{
	struct device_data *dev;
	printf("\n");
	printf("%s\n", _("Recent SATA AHCI link activity statistics"));
	printf("%s\n", _("Active\tPartial\tSlumber\tDevice name"));
	dev = devices;
	while (dev) {
		if (dev->total > 0)
			printf("%5.1f%%\t%5.1f%%\t%5.1f%%\t%s\n", 
				100.0*(dev->active - dev->previous_active) / (0.00001 + dev->total - dev->previous_total),
				100.0*(dev->partial - dev->previous_partial) / (0.00001 + dev->total - dev->previous_total),
				100.0*(dev->slumber - dev->previous_slumber) / (0.00001 + dev->total - dev->previous_total),
				dev->human_name);
		dev = dev->next;
	}

}

void ahci_activity_hint(void)
{
	int total_active = 0;
	int pick;
	struct device_data *dev;
	dev = devices;
	while (dev) {
		if (dev->active-1 > dev->previous_active && dev->active)
			total_active++;
		dev = dev->next;
	}
	if (!total_active)
		return;

	pick = rand() % total_active;
	total_active = 0;
	dev = devices;
	while (dev) {
		if (dev->active-1 > dev->previous_active && dev->active) {
			if (total_active == pick) {
				char ahci_hint[8000];
				sprintf(ahci_hint, _("A SATA device is active %1.1f%% of the time:\n%s"),
				 100.0*(dev->active - dev->previous_active) / (0.00001 + dev->total - dev->previous_total),
				dev->human_name);
				add_suggestion(ahci_hint,
				15, 'S', _(" S - SATA Link Power Management "), activate_alpm);
			}
			total_active++;
		}
		dev = dev->next;
	}

}
