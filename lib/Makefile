CFG_OSTYPE := $(shell uname -s)
CFG_CPUTYPE := $(shell uname -m)

CFG_GCC_CFLAGS :=
CFG_GCC_LINK_FLAGS :=
CFG_CERTIFICATE_NAME := RustDbg

ifeq ($(CFG_OSTYPE), Darwin)
  CFG_LIB := librustdbg.dylib
  CFG_GCC_CFLAGS += -fPIC
  CFG_GCC_LINK_FLAGS += -fPIC -ldl -lpthread -ludis86 -framework Security
  CFG_VALGRIND := $(shell which valgrind)
  ifdef CFG_VALGRIND
    CFG_VALGRIND += --leak-check=full --quiet --vex-iropt-level=0
  endif
  CFG_GCC := 1
endif

SOURCE_CPP := $(wildcard src/*.cpp)
SOURCE_HDR := $(wildcard src/*.h)
SOURCE_OBJ := $(patsubst %.cpp, %.o, $(SOURCE_CPP))

TEST_CPP := $(wildcard src/test/*.cpp)
TEST_HDR := $(wildcard src/test/*.h)
TEST_OBJ := $(patsubst %.cpp, %.x86, $(TEST_CPP))

ifdef CFG_GCC
  CFG_INFO := $(info cfg: using gcc)
  CFG_GCC_CFLAGS += -Wall -Werror -fno-rtti -fno-exceptions -g
  CFG_GCC_LINK_FLAGS += -g
  CFG_COMPILE_C = g++ $(CFG_GCC_CFLAGS) -c -o $(1) $(2)
  CFG_COMPILE_C_TEST = g++ $(CFG_GCC_CFLAGS) -L. -lrustdbg -o $(1) $(2)
  CFG_LINK_C_SHARED = g++ -shared $(CFG_GCC_LINK_FLAGS) -o $(1)
  CFG_LINK_C = g++ $(CFG_GCC_LINK_FLAGS) -o $(1)
  CFG_DEPEND_C = g++ $(CFG_GCC_CFLAGS) -MT "$(1)" -MM $(2)
else
  CFG_ERR := $(error please try on a system with gcc)
endif

all: $(CFG_LIB) check
	
$(CFG_LIB): $(SOURCE_OBJ) $(SOURCE_HDR)
	g++ -shared $(CFG_GCC_LINK_FLAGS) -ldl -o $(CFG_LIB) $(SOURCE_OBJ)
	
%.o: %.cpp
	g++ $(CFG_GCC_CFLAGS) -c -o $@ $<
	
%.x86: %.cpp $(CFG_LIB)
	g++ $(CFG_GCC_CFLAGS) -L. -lrustdbg -lreadline -o $@ $<
	codesign -s $(CFG_CERTIFICATE_NAME) $@

check: $(TEST_OBJ)

clean:
	rm -f $(SOURCE_OBJ)
	rm -f $(TEST_OBJ)