CFLAGS		?= -std=gnu99 -Os -Wall
CXXFLAGS	?= -std=gnu++11 -Os -Wall

VERSION ?= $(shell git describe --always)

ROMFS_SRC ?= ../../romfs

ifeq ($(OS),Windows_NT)
	TARGET_OS := WINDOWS
	DIST_SUFFIX := windows
	ARCHIVE_CMD := 7z a
	ARCHIVE_EXTENSION := zip
	TARGET := mkromfs.exe
	TARGET_CFLAGS := -DMKROMFS -mno-ms-bitfields -Itclap -I$(ROMFS_SRC) -I. -DVERSION=\"$(VERSION)\" -D__NO_INLINE__
	TARGET_LDFLAGS := -Wl,-static -static-libgcc
	TARGET_CXXFLAGS := -Itclap -I$(ROMFS_SRC) -I. -DVERSION=\"$(VERSION)\" -D__NO_INLINE__
	CC=gcc
	CXX=g++
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
		CC=gcc
		CXX=g++
		TARGET_CFLAGS   = -DMKROMFS -std=gnu99 -Os -Wall -Itclap -I$(ROMFS_SRC) -I. -D$(TARGET_OS) -DVERSION=\"$(VERSION)\" -D__NO_INLINE__
		TARGET_CXXFLAGS = -std=gnu++11 -Os -Wall -Itclap -I$(ROMFS_SRC) -I. -D$(TARGET_OS) -DVERSION=\"$(VERSION)\" -D__NO_INLINE__
	endif
	ifeq ($(UNAME_S),Darwin)
		TARGET_OS := OSX
		DIST_SUFFIX := osx
		CC=clang
		CXX=clang++
		TARGET_CFLAGS   = -DMKROMFS -std=gnu99 -Os -Wall -Itclap -I$(ROMFS_SRC) -I. -D$(TARGET_OS) -DVERSION=\"$(VERSION)\" -D__NO_INLINE__ -mmacosx-version-min=10.7 -arch x86_64
		TARGET_CXXFLAGS = -std=gnu++11 -Os -Wall -Itclap -I$(ROMFS_SRC) -I. -D$(TARGET_OS) -DVERSION=\"$(VERSION)\" -D__NO_INLINE__ -mmacosx-version-min=10.7 -arch x86_64 -stdlib=libc++
		TARGET_LDFLAGS  = -arch x86_64 -stdlib=libc++
	endif
	ARCHIVE_CMD := tar czf
	ARCHIVE_EXTENSION := tar.gz
	TARGET := mkromfs
endif

OBJ             := mkromfs.o \
                   romfs.o
                   				   
VERSION ?= $(shell git describe --always)

.PHONY: all clean

all: $(TARGET)

$(TARGET):
	@echo "Building mkromfs ..."
	$(CC) $(TARGET_CFLAGS) -c $(ROMFS_SRC)/romfs.c -o romfs.o
	$(CC) $(TARGET_CFLAGS) -c mkromfs.c -o mkromfs.o
	$(CXX) $(TARGET_CFLAGS) -o $(TARGET) $(OBJ) $(TARGET_LDFLAGS)
	
clean:
	@rm -f *.o
	@rm -f romfs/*.o
	@rm -f $(TARGET)