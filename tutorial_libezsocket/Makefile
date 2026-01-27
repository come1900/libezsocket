# Copyright (C) 2012  WuJunjie(Joy.Woo@hotmail.com)

# platforms:
#  linux-x86-32
#  uclinux-s3c2510

PLATFORM = linux
#PLATFORM = linux-x86-32
#PLATFORM = linux-hi3515
#PLATFORM = linux-arm
#PLATFORM = osx
PLATFORM = linux-mipsel-openwrt-linux

VERSION =	1.0
DIFF_VERSION =	0.9

DIST =	get_pub_ip-$(PLATFORM) \
	form_post_cli-$(PLATFORM) \
	porsts_cli-$(PLATFORM) \
	faceaip-$(PLATFORM) \
	simple_rtsp_cli-$(PLATFORM) \
	ez_telnet-$(PLATFORM) \
	rtsp_valid-$(PLATFORM) \
	tcp_svr_dumpdata-$(PLATFORM) \
	getStreamPlayUrl-$(PLATFORM) \
	getPlayUrlMeye-$(PLATFORM) \
	platform_chiper-$(PLATFORM)

DIST += rtp_rcv_udp-$(PLATFORM) \
	touch-cli-native-$(PLATFORM) \
	touch-cli-native-multi-$(PLATFORM) \
	touch-svr-native-$(PLATFORM)

CFLAGS =	-Wall -g -O2 -MD
CFLAGS += -D_DEBUG #debugme

#include         Makefile.version
#include         ~/daily/src/library/Makefile.Defines/Makefile.Defines.$(PLATFORM)
include ../../Makefile.Defines/Makefile.Defines.$(PLATFORM)

CLEAN_FILES += *actual* *rewrite* a.txt

#���⹦����
#

CPPFLAGS =	$(CFLAGS) 

EZLIBS_BASEDIR_LIBS=$(HOME)/libs
CFLAGS += -I${EZLIBS_BASEDIR_LIBS}/include/ezsocket
LIBS += -L${EZLIBS_BASEDIR_LIBS}/lib -lezsocket-$(PLATFORM)

CFLAGS += -I${EZLIBS_BASEDIR_LIBS}/include/
CFLAGS += -I${EZLIBS_BASEDIR_LIBS}/include/ezutil
LIBS += -L${EZLIBS_BASEDIR_LIBS}/lib -lezutil-$(PLATFORM)

LIBS += -lpthread

all: $(DIST)

get_pub_ip-$(PLATFORM):get_pub_ip.o
	$(CPP) -o $@ $^ $(LIBS)
rtp_rcv_udp-$(PLATFORM):rtp_rcv_udp.o
	$(CPP) -o $@ $^ $(LIBS)
platform_chiper-$(PLATFORM):platform_chiper.o
	$(CPP) -o $@ $^ $(LIBS)
simple_rtsp_cli-$(PLATFORM):simple_rtsp_cli.o
	$(CPP) -o $@ $^ $(LIBS)
ez_telnet-$(PLATFORM):ez_telnet.o
	$(CPP) -o $@ $^ $(LIBS)
rtsp_valid-$(PLATFORM):rtsp_valid.o
	$(CPP) -o $@ $^ $(LIBS)
tcp_svr_dumpdata-$(PLATFORM):tcp_svr_dumpdata.o
	$(CPP) -o $@ $^ $(LIBS)
getStreamPlayUrl-$(PLATFORM):getStreamPlayUrl.o
	$(CPP) -o $@ $^ $(LIBS)
getPlayUrlMeye-$(PLATFORM):getPlayUrlMeye.o
	$(CPP) -o $@ $^ $(LIBS)
faceaip-$(PLATFORM):faceaip.o
	$(CPP) -o $@ $^ $(LIBS)
form_post_cli-$(PLATFORM):form_post_cli.o
	$(CPP) -o $@ $^ $(LIBS)
porsts_cli-$(PLATFORM):porsts_cli.o
	$(CPP) -o $@ $^ $(LIBS)
touch-cli-native-$(PLATFORM):touch-cli-native.o
	$(CPP) -o $@ $^ $(LIBS)
touch-cli-native-multi-$(PLATFORM):touch-cli-native-multi.o
	$(CPP) -o $@ $^ $(LIBS)
touch-svr-native-$(PLATFORM):touch-svr-native.o
	$(CPP) -o $@ $^ $(LIBS)

FILES =		*.h *.cpp Makefile*

tar:	clean
		tar -czf $(DIST).tgz $(FILES)

-include	*.d

CLEAN_FILES = $(DIST) *.o *.gdb *.d *.cfbk *.tgz

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

.cpp.o:
	$(CPP) -c $(CFLAGS) $< -o $@

clean: 
	rm -f $(CLEAN_FILES)

