#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
#
# Makefile - Z:\Dropbox\stock\trunk\code\library\linux_device\libplatform\test\
#
# Copyright (C) 2013 ezlibs.com, All Rights Reserved.
#
# $Id: Makefile 1 2013-06-13 10:05:26 WuJunjie Exp $
#
# explain
#      exp
#
# Update:
#    2013-06-13 10:05 WuJunjie Create
#
#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

PLATFORM = linux
#PLATFORM = linux-x86-32
#PLATFORM = linux-hi3515
#PLATFORM = linux-arm
#PLATFORM = linux-x86-64-centos5
#PLATFORM = linux-am335x

VERSION =	1.0
DIFF_VERSION =	0.9

DIST =		exef

CFLAGS =	-Wall -g -O2 -MD
CFLAGS += -D_DEBUG #debugme

#include         Makefile.Defines.$(PLATFORM)
include ../../Makefile.Defines/Makefile.Defines.$(PLATFORM)


#CFLAGS += -DLINUX
CFLAGS	+= -I../src
CFLAGS += -Wall -O2 -Os
CFLAGS += -D_REENTRANT -msoft-float

#LDFLAGS += -L../lib/\
#	-lezthread-linux-x86-32
EZLIBS_BASEDIR_LIBS=$(HOME)/libs
CFLAGS += -I${EZLIBS_BASEDIR_LIBS}/include/ezutil
LIBS += -L${EZLIBS_BASEDIR_LIBS}/lib -lezutil-$(PLATFORM)

LDFLAGS += $(LIBS) -lpthread

AFLAGS += -r

#LIB_OBJS = ../src/api_exa.o
PREFIX_BASEDIR_SRC=../src
include $(PREFIX_BASEDIR_SRC)/Makefile.SrcLists

TARGET = get_pub_ip-$(PLATFORM)\
		nvp_req-$(PLATFORM)\
		pstAddAccount-$(PLATFORM)\
		pstAddDevice-$(PLATFORM)\
		getStreamPlayUrl-$(PLATFORM)\
		sms_ezlibs_reg-$(PLATFORM)\
		simple_rtsp_cli-$(PLATFORM)\
		simple_rtsp_valid-$(PLATFORM)\
		http_cli-$(PLATFORM)\
		platform_chiper-$(PLATFORM)

# test_net_gate_way

all: $(TARGET)
#	./test_net_work_interface

get_pub_ip-$(PLATFORM):  get_pub_ip.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)
	#$(STRIP) $@
	
platform_chiper-$(PLATFORM):  platform_chiper.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)
	#$(STRIP) $@
	
simple_rtsp_cli-$(PLATFORM):simple_rtsp_cli.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)
simple_rtsp_valid-$(PLATFORM):simple_rtsp_valid.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)
http_cli-$(PLATFORM):http_cli.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)

sms_ezlibs_reg-$(PLATFORM):sms_ezlibs_reg.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)
nvp_req-$(PLATFORM):nvp_req.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)
pstAddAccount-$(PLATFORM):pstAddAccount.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)
pstAddDevice-$(PLATFORM):pstAddDevice.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)

getStreamPlayUrl-$(PLATFORM):getStreamPlayUrl.o $(LIB_OBJS)
	$(CPP) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $^ -o $@

.cpp.o:
	$(CPP) -c $(CFLAGS) $^ -o $@

clean:
	$(RM) $(TARGET) $(LIB_OBJS) *.o *.cfbk *.d;

dbg:$(TARGET)
	cp $(TARGET) /home/share

