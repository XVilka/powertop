BINDIR=${HOME}/build/cross/sys-root/usr/bin
LOCALESDIR=${HOME}/build/cross/sys-root/usr/share/locale
MANDIR=${HOME}/build/cross/sys-root/usr/share/man/man1
WARNFLAGS=-Wall -W -Wshadow
CFLAGS= -static -g ${_XXCFLAGS} -D__ARM__ -I ${HOME}/build/cross/sys-root/usr/include -L ${HOME}/build/cross/sys-root/usr/lib ${WARNFLAGS}
CC=arm-android-linux-uclibcgnueabi-gcc
STRIP=arm-android-linux-uclibcgnueabi-sstrip

OBJS = powertop.o config.o process.o misctips.o bluetooth.o display.o suggestions.o wireless.o cpufreq.o \
	sata.o xrandr.o ethernet.o cpufreqstats.o usb.o urbnum.o intelcstates.o armcstates.o wifi-new.o perf.o \
	alsa-power.o ahci-alpm.o dmesg.o
	

powertop: $(OBJS) Makefile powertop.h
	$(CC) ${CFLAGS} $(LDFLAGS) $(OBJS) -lncurses -lc -o powertop
	$(STRIP) powertop
	@(cd po/ && $(MAKE))

powertop.8.gz: powertop.8
	gzip -c $< > $@

install: powertop powertop.8.gz
	mkdir -p ${DESTDIR}${BINDIR}
	cp powertop ${DESTDIR}${BINDIR}
	mkdir -p ${DESTDIR}${MANDIR}
	cp powertop.8.gz ${DESTDIR}${MANDIR}
	@(cd po/ && env LOCALESDIR=$(LOCALESDIR) DESTDIR=$(DESTDIR) $(MAKE) $@)

# This is for translators. To update your po with new strings, do :
# svn up ; make uptrans LG=fr # or de, ru, hu, it, ...
uptrans:
	@(cd po/ && env LG=$(LG) $(MAKE) $@)

clean:
	rm -f *~ powertop powertop.8.gz po/powertop.pot DEADJOE svn-commit* *.o *.orig 
	@(cd po/ && $(MAKE) $@)


dist:
	rm -rf .svn po/.svn DEADJOE po/DEADJOE todo.txt Lindent svn-commit.* dogit.sh git/ *.rej *.orig
