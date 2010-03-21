LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=	\
	powertop.c		\
	config.c		\
	process.c		\
	misctips.c		\
	bluetooth.c		\
	display.c		\
	suggestions.c	\
	wireless.c		\
	cpufreq.c		\
	sata.c			\
	xrandr.c		\
	ethernet.c		\
	cpufreqstats.c	\
	usb.c			\
	urbnum.c		\
	intelcstates.c	\
	armcstates.c	\
	wifi-new.c		\
	perf.c			\
	alsa-power.c	\
	ahci-alpm.c		\
	dmesg.c

 LOCAL_STATIC_LIBRARIES :=	\
 		libslang-static
# 		libglib-static		\

 LOCAL_C_INCLUDES +=			\
 	$(TARGET_OUT_HEADERS)/libslang
# 	$(TARGET_OUT_HEADERS)/glib
# 	$(LOCAL_PATH)/android 	\

# 	external/glib			\
# 	external/libslang		\


LOCAL_MODULE:= powertop

 LOCAL_CFLAGS +=		\
	-DDEFAULT_TERM=\"xterm\" \
	-D__ARM__			\
	-DUSE_SLANG=1		\
	-DIS_ANDROID=1
# 	-DSAVERDIR=\"/system/bin/\"		\
# 	-DSYSCONFDIR=\"/sdcard/etc/mc/\"	\
# 	-DDATADIR=\"/sdcard/etc/mc/\"		\
# 	-DSHELL=\"/system/bin/sh\"			\
# 	-DDEFAULT_TERM=\"xterm\"			\
# 	-DREAL_UNIX_SYSTEM=1			\
# 	-Wcomment -Wdeclaration-after-statement -Wformat -Wimplicit-function-declaration -Wimplicit-int -Wmissing-braces -Wmissing-declarations -Wmissing-parameter-type -Wmissing-prototypes -Wnested-externs -Wno-long-long -Wno-unreachable-code -Wparentheses -Wpointer-sign -Wreturn-type -Wshadow -Wsign-compare -Wswitch -Wuninitialized -Wunused-function -Wunused-label -Wunused-parameter -Wunused-value -Wunused-variable -Wwrite-strings  -g
# 	-std=gnuc \
# 	-Wall			
# 	-std=gnu99
# 	-D_GNU_SOURCE	\
# 	-D_THREAD_SAFE	\
# 	-Werror

include $(BUILD_EXECUTABLE)
