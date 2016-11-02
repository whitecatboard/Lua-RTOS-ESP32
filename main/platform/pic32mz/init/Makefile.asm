BINDIR := /usr/local/gnu-mips/bin

SRCS=$(wildcard $(FOLDER)/*.s)

APP=$(BINDIR)/mips-elf-cpp -DLOCORE -P -IFreeRTOS/Source/include/platform/pic32mz -IFreeRTOS/Source/portable/pic32mz -Ilibc/platform/pic32mz/include -Iinclude/platform/pic32mz -D__LANGUAGE_ASSEMBLY__
AAS=$(BINDIR)/mips-elf-as -mdsp -mips32r2 -EL -mfp64 -G 0 -Os -o 

asm: $(SRCS)
	$(foreach SRC,$(SRCS), ($(APP) $(SRC) | $(AAS) $(patsubst %.s,%.o,$(SRC)));)
