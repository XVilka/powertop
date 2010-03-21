/*
 * Copyright 2009, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * Portions borrowed from the "perf" tool (C) Ingo Molnar and others
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

#define _GNU_SOURCE        /* or _BSD_SOURCE or _SVID_SOURCE */
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fcntl.h>

#include "perf_event.h"

#include "powertop.h"


/* some people have stale headers */
#ifndef __NR_perf_event_open
#if defined(__i386__) || defined(__ARM__)
#define __NR_perf_event_open	336
#endif
#if __x86_64__
#define __NR_perf_event_open	298
#endif
#endif

static int perf_fd = -1;
static void * perf_mmap;
static void * data_mmap;

static struct perf_event_mmap_page *pc;

static inline int
sys_perf_event_open(struct perf_event_attr *attr,
                      pid_t pid, int cpu, int group_fd,
                      unsigned long flags)
{
	attr->size = sizeof(*attr);
	return syscall(__NR_perf_event_open, attr, pid, cpu,
			group_fd, flags);
}

static int this_trace = 0;

static int get_trace_type(void)
{
	FILE *file;
	char line[4096];
	file = fopen("/sys/kernel/debug/tracing/events/vfs/dirty_inode/id", "r");
	if (!file)
		return 0;

	if (fgets(line, 4095, file) == NULL)
		return 0;
	
	this_trace = strtoull(line, NULL, 10);
	fclose(file);
	return this_trace;
}

static void create_perf_event(void)
{
	struct perf_event_attr attr;
	int ret;

	struct {
		__u64 count;
		__u64 time_enabled;
		__u64 time_running;
		__u64 id;
	} read_data;

	memset(&attr, 0, sizeof(attr));

	attr.read_format	= PERF_FORMAT_TOTAL_TIME_ENABLED |
				  PERF_FORMAT_TOTAL_TIME_RUNNING |
				  PERF_FORMAT_ID;

	attr.sample_freq 	= 0;
	attr.sample_period	= 1;
	attr.sample_type	|= PERF_SAMPLE_RAW;

	attr.mmap		= 1;
	attr.comm		= 1;
	attr.inherit		= 0;
	attr.disabled		= 1;

	attr.type		= PERF_TYPE_TRACEPOINT;
	attr.config		= get_trace_type();

	if (attr.config <= 0)
		return;

	perf_fd = sys_perf_event_open(&attr, -1, 0, -1, 0);

	if (perf_fd < 0) {
		fprintf(stderr, "Perf syscall failed: %i / %i (%s)\n", perf_fd, errno, strerror(errno));
		return;
	}
	if (read(perf_fd, &read_data, sizeof(read_data)) == -1) {
		perror("Unable to read perf file descriptor\n");
		exit(-1);
	}

	fcntl(perf_fd, F_SETFL, O_NONBLOCK);

	perf_mmap = mmap(NULL, (128+1)*getpagesize(),
				PROT_READ | PROT_WRITE, MAP_SHARED, perf_fd, 0);
	if (perf_mmap == MAP_FAILED) {
		fprintf(stderr, "failed to mmap with %d (%s)\n", errno, strerror(errno));
		return;
	}

	ret = ioctl(perf_fd, PERF_EVENT_IOC_ENABLE);

	if (ret < 0)
		fprintf(stderr, "failed to enable perf \n");

	pc = perf_mmap;
	data_mmap = perf_mmap + getpagesize();


}


void start_data_dirty_capture(void)
{
	create_perf_event();
}

void end_data_dirty_capture(void)
{
	if (perf_fd >= 0)
		close(perf_fd);
	perf_fd = -1;
}

struct trace_entry {
	__u32			size;
	unsigned short		type;
	unsigned char		flags;
	unsigned char		preempt_count;
	int			pid;
	int			tgid;
};

#define TASK_COMM_LEN 16
struct dirty_inode { 
	char comm[TASK_COMM_LEN];
	int  pid;
	char dev[16];
	char file[32];
};


struct sample_event{
	struct perf_event_header        header;
	struct trace_entry		trace;
	struct dirty_inode		inode;
};


static void parse_event(void *ptr, int verbose)
{
	char line[8192];
	char pid[14];
	int suggested = 0;
	struct sample_event *event = ptr;
	if (event->trace.type != this_trace)
		return;

	if (event->trace.size < sizeof(struct dirty_inode)) 
		return;

	if (event->inode.pid == 0)
		return;

	if (strcmp(event->inode.comm, "powertop") == 0)
		return;
	/*
	 * btrfs kernel threads are internal and only
	 * do IO on behalf of others that also got recorded
	 */

	if (strcmp(event->inode.comm, "btrfs-") == 0)
		return;
	/*
	 * don't record "IO" to tmpfs or /proc
	 */
	if (strcmp(event->inode.dev, "tmpfs") == 0)
		return;
	if (strcmp(event->inode.dev, "proc") == 0)
		return;
	if (strcmp(event->inode.dev, "pipefs") == 0)
		return;
	if (strcmp(event->inode.dev, "anon_inodefs") == 0)
		return;

	sprintf(pid, "%i", event->inode.pid);
	sprintf(line, "%s", event->inode.comm);
	push_line_pid(line, 0, 1, pid);

	if (!suggested && strcmp(event->inode.file, "?")) {
		suggested = 1;
		sprintf(line,_("The program '%s' is writing to file '%s' on /dev/%s.\nThis prevents the disk from going to powersave mode."),
			event->inode.comm, event->inode.file, event->inode.dev);
		add_suggestion(line, 30, 0, NULL, NULL);
	}
	if (verbose) 
		printf(_("The application '%s' is writing to file '%s' on /dev/%s\n"),
			event->inode.comm, event->inode.file, event->inode.dev);

}

void parse_data_dirty_buffer(void)
{
	struct perf_event_header *header;
	int i = 0;

	if (perf_fd < 0)
		return;

	if (dump)
		printf(_("Disk accesses:\n"));

	while (pc->data_tail != pc->data_head && i++ < 5000) {
		while (pc->data_tail >= 128U * getpagesize())
			pc->data_tail -= 128 * getpagesize();

		header = data_mmap + pc->data_tail;

		if (header->size == 0)
			break;

		pc->data_tail += header->size;

		while (pc->data_tail >= 128U * getpagesize())
			pc->data_tail -= 128 * getpagesize();

		if (header->type == PERF_RECORD_SAMPLE)
			parse_event(header, dump);
	}
	pc->data_tail = pc->data_head;
}
