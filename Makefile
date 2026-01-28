#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
#
# Makefile - \library\libezthread\Build\
#
# Copyright (C) 2011 ezlibs.com, All Rights Reserved.
#
# $Id: Makefile 1 2011-10-22 08:14:10 WuJunjie Exp $
#
# explain
#      for joy.woo only
#      use build/Makefile pls
#
# Update:
#    2011-10-22 08:14 WuJunjie Create
#
#-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
PUB_MAKE_FILE_PREFIX =	../lazaru/Makefile.Defines
include		$(PUB_MAKE_FILE_PREFIX)/Makefile.Batch.Build

# define default platform
ifndef PLATFORM
PLATFORM = linux
endif
#PLATFORM = linux-mipsel-openwrt-linux

VERSION="1.0.0"

# CPP, CFLAGS, LIBS
include		$(PUB_MAKE_FILE_PREFIX)/Makefile.Defines.$(PLATFORM)
PREFIX_BASEDIR_SRC=./src
include $(PREFIX_BASEDIR_SRC)/Makefile.SrcLists
#LIB_OBJS =	\
#		./src/api_exa.o

EZLIBS_BASEDIR_LIBS=$(HOME)/libs
CFLAGS += -I${EZLIBS_BASEDIR_LIBS}/include
#LIBS += -L${EZLIBS_BASEDIR_LIBS}/lib -lezutil-$(PLATFORM)

PREFIX =	$(HOME)/libs
INS_DIR_LIB = $(PREFIX)/lib
INS_DIR_INC = $(PREFIX)/include/ezsocket

LIB_TARGET = libezsocket-$(PLATFORM).a

TARGET	= $(LIB_TARGET)
all: $(TARGET)

$(LIB_TARGET): $(LIB_OBJS)
	#mkdir -p $(LIBDIR)
	$(RM) $@;
	$(AR) $(AR_FLAG) $@ $^
	$(RANLIB) $@

install:	all 
		@mkdir -p $(INS_DIR_LIB)
		cp $(LIB_TARGET) $(INS_DIR_LIB)
		@mkdir -p $(INS_DIR_INC)
		cp ./src/*.h $(INS_DIR_INC)
.c.o:
	$(CC) -c $(CFLAGS) $^ -o $@

.cpp.o:
	$(CPP) -c $(CFLAGS) $^ -o $@

clean:
	$(RM) $(TARGET) $(LIB_OBJS) *.a *.cfbk ./lib/* ./include/*
	find . -name "*.d" |xargs rm -f
	find . -name "*.cfbk" |xargs rm -f
	make -C tutorial_libezsocket clean
