CFLAGS		?= -std=gnu99 -Os -Wall
CXXFLAGS	?= -std=gnu++11 -Os -Wall

ifeq ($(OS),Windows_NT)
	TARGET_OS := WINDOWS
	DIST_SUFFIX := windows
	ARCHIVE_CMD := 7z a
	ARCHIVE_EXTENSION := zip
	TARGET := mkspiffs.exe
	TARGET_CFLAGS := -mno-ms-bitfields
	TARGET_LDFLAGS := -Wl,-static -static-libgcc

else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		TARGET_OS := LINUX
		UNAME_P := $(shell uname -p)
		ifeq ($(UNAME_P),x86_64)
			DIST_SUFFIX := linux64
		endif
		ifneq ($(filter %86,$(UNAME_P)),)
			DIST_SUFFIX := linux32
		endif
	endif
	ifeq ($(UNAME_S),Darwin)
		TARGET_OS := OSX
		DIST_SUFFIX := osx
		CC=clang
		CXX=clang++
		TARGET_CFLAGS   = -mmacosx-version-min=10.7 -arch i386 -arch x86_64
		TARGET_CXXFLAGS = -mmacosx-version-min=10.7 -arch i386 -arch x86_64 -stdlib=libc++
		TARGET_LDFLAGS  = -arch i386 -arch x86_64 -stdlib=libc++
	endif
	ARCHIVE_CMD := tar czf
	ARCHIVE_EXTENSION := tar.gz
	TARGET := mkspiffs
endif

VERSION ?= $(shell git describe --always)

OBJ		:= main.o \
		   spiffs/spiffs_cache.o \
		   spiffs/spiffs_check.o \
		   spiffs/spiffs_gc.o \
		   spiffs/spiffs_hydrogen.o \
		   spiffs/spiffs_nucleus.o \

INCLUDES := -Itclap -Ispiffs -I.

CFLAGS   += $(TARGET_CFLAGS)
CXXFLAGS += $(TARGET_CXXFLAGS)
LDFLAGS  += $(TARGET_LDFLAGS)

CPPFLAGS += $(INCLUDES) -D$(TARGET_OS) -DVERSION=\"$(VERSION)\" -D__NO_INLINE__

DIST_NAME := mkspiffs-$(VERSION)-$(DIST_SUFFIX)
DIST_DIR := $(DIST_NAME)
DIST_ARCHIVE := $(DIST_NAME).$(ARCHIVE_EXTENSION)

.PHONY: all clean dist

all: $(TARGET)

dist: test $(DIST_ARCHIVE)

$(DIST_ARCHIVE): $(TARGET) $(DIST_DIR)
	cp $(TARGET) $(DIST_DIR)/
	$(ARCHIVE_CMD) $(DIST_ARCHIVE) $(DIST_DIR)

$(TARGET): $(OBJ)
	$(CXX) $^ -o $@ $(LDFLAGS)
	strip $(TARGET)

$(DIST_DIR):
	@mkdir -p $@

clean:
	@rm -f *.o
	@rm -f spiffs/*.o
	@rm -f $(TARGET)

SPIFFS_TEST_FS_CONFIG := -s 0x100000 -p 512 -b 0x2000

test: $(TARGET)
	ls -1 spiffs > out.list0
	./mkspiffs -c spiffs $(SPIFFS_TEST_FS_CONFIG) out.spiffs | sort | sed s/^\\/// > out.list1
	./mkspiffs -u spiffs_u $(SPIFFS_TEST_FS_CONFIG) out.spiffs | sort | sed s/^\\/// > out.list_u
	./mkspiffs -l $(SPIFFS_TEST_FS_CONFIG) out.spiffs | cut -f 2 | sort | sed s/^\\/// > out.list2
	diff --strip-trailing-cr out.list0 out.list1
	diff --strip-trailing-cr out.list0 out.list2
	diff spiffs spiffs_u
	rm -f out.{list0,list1,list2,list_u,spiffs}
	rm -R spiffs_u
