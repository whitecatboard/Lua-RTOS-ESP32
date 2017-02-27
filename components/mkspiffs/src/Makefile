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
		CC=gcc
		CXX=g++
		TARGET_CFLAGS   = -std=gnu99 -Os -Wall -Itclap -Ispiffs -I. -D$(TARGET_OS) -DVERSION=\"$(VERSION)\" -D__NO_INLINE__
		TARGET_CXXFLAGS = -std=gnu++11 -Os -Wall -Itclap -Ispiffs -I. -D$(TARGET_OS) -DVERSION=\"$(VERSION)\" -D__NO_INLINE__
	endif
	ifeq ($(UNAME_S),Darwin)
		TARGET_OS := OSX
		DIST_SUFFIX := osx
		CC=clang
		CXX=clang++
		TARGET_CFLAGS   = -std=gnu99 -Os -Wall -Itclap -Ispiffs -I. -D$(TARGET_OS) -DVERSION=\"$(VERSION)\" -D__NO_INLINE__ -mmacosx-version-min=10.7 -arch x86_64
		TARGET_CXXFLAGS = -std=gnu++11 -Os -Wall -Itclap -Ispiffs -I. -D$(TARGET_OS) -DVERSION=\"$(VERSION)\" -D__NO_INLINE__ -mmacosx-version-min=10.7 -arch x86_64 -stdlib=libc++
		TARGET_LDFLAGS  = -arch x86_64 -stdlib=libc++
	endif
	ARCHIVE_CMD := tar czf
	ARCHIVE_EXTENSION := tar.gz
	TARGET := mkspiffs
endif

OBJ             := main.o \
                   spiffs/spiffs_cache.o \
                   spiffs/spiffs_check.o \
                   spiffs/spiffs_gc.o \
                   spiffs/spiffs_hydrogen.o \
                   spiffs/spiffs_nucleus.o \
				   
VERSION ?= $(shell git describe --always)

.PHONY: all clean

all: $(TARGET)

$(TARGET):
	@echo "Building mkspiffs ..."
	$(CC) $(TARGET_CFLAGS) -c spiffs/spiffs_cache.c -o spiffs/spiffs_cache.o
	$(CC) $(TARGET_CFLAGS) -c spiffs/spiffs_check.c -o spiffs/spiffs_check.o
	$(CC) $(TARGET_CFLAGS) -c spiffs/spiffs_gc.c -o spiffs/spiffs_gc.o
	$(CC) $(TARGET_CFLAGS) -c spiffs/spiffs_hydrogen.c -o spiffs/spiffs_hydrogen.o
	$(CC) $(TARGET_CFLAGS) -c spiffs/spiffs_nucleus.c -o spiffs/spiffs_nucleus.o
	$(CXX) $(TARGET_CXXFLAGS) -c main.cpp -o main.o
	$(CXX) $(TARGET_CFLAGS) -o $(TARGET) $(OBJ) $(TARGET_LDFLAGS)
	
clean:
	@rm -f *.o
	@rm -f spiffs/*.o
	@rm -f $(TARGET)