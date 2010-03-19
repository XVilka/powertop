/*
 * Copyright 2010, XVilka
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
 * 	XVilka <xvilka@gmail.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#include "powertop.h"

#if defined(__ARM__)
/**
 * print_arm_cstates() - Prints the list of supported C-states.
 *
 * This functions uses standard sysfs interface of the cpuidle framework
 * to extract the information of the C-states supported by the Linux
 * kernel. 
 **/
void print_arm_cstates(void)
{
	DIR *dir;
	struct dirent *entry;

	dir = opendir("/sys/devices/system/cpu/cpu0/cpuidle");

	if (dir) {
		printf(_("Supported C-states : "));

		while ((entry = readdir(dir))) {
			if (strlen(entry->d_name) < 3)
				continue;

			printf("C%s ", entry->d_name);
		}
		printf("\n");

		closedir(dir);
	}
}
#endif
