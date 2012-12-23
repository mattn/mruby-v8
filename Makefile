GEM := mruby-v8

include $(MAKEFILE_4_GEM)

CFLAGS += -I$(MRUBY_ROOT)/include
MRUBY_CFLAGS += -I$(MRUBY_ROOT)/include
ifeq ($(OS),Windows_NT)
MRUBY_LIBS += -lv8
else
MRUBY_LIBS += -lv8
endif

GEM_C_FILES := $(wildcard $(SRC_DIR)/*.c)
GEM_OBJECTS := $(patsubst %.c, %.o, $(GEM_C_FILES))

GEM_C_FILE += src/v8wrap.cc
GEM_OBJECTS += src/v8wrap.o

gem-all : $(GEM_OBJECTS) gem-c-files

gem-clean : gem-clean-c-files

src/v8wrap.o : src/v8wrap.cc
	g++ -g -c -o src/v8wrap.o -Iinclude src/v8wrap.cc
