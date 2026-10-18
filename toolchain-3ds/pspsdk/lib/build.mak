# platinum3ds stand-in for PSPSDK build.mak: object rules only (final link is done by the 3DS Makefile).
CC = psp-gcc
CXX = psp-g++
AS = psp-gcc
LD = psp-gcc
AR = psp-ar
override CFLAGS := $(filter-out -G0,$(CFLAGS)) $(addprefix -I,$(INCDIR))
CXXFLAGS += $(CFLAGS)
ASFLAGS += $(CFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%.o: %.S
	$(CC) $(ASFLAGS) -c -o $@ $<
all:
	@echo "platinum3ds: PSP link step skipped"
