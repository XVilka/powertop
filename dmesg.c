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
#include <sys/klog.h>

int klogctl(int type, char *bufp, int len);
#include "powertop.h"

/* static arrays are not nice programming.. but they're easy */
static char *dmesg_content;

static void read_dmesg(void)
{
	int size;

	if (dmesg_content)
		return;

	size = klogctl(10, NULL, 0);

	if (size <= 0)
		return;

	dmesg_content = calloc(size + 1, 1);

	klogctl(3, dmesg_content, size);
}

void suggest_on_dmesg(char *string, char *comment, int weight)
{
	read_dmesg();

	if (strstr(dmesg_content, string))
		add_suggestion(comment, weight, 0, NULL, NULL);
}
