# ----------------------------------------------------------
#   |  author  : wenqvip                  |     \|/     |
#   |  email   : wenqvip@gmail.com        |    --+--    |
#   |  version : 0.3.2                    |     /|\     |
# ----------------------------------------------------------

# if you define OUT with suffix '.a', a static library will be made;
# with suffix '.so', a dynamic library will be made;
# any others, a excutable file will be made.
OUT=kcp-echo

CC=clang
CXX=clang
LD=clang

# you change here
SRCS=$(wildcard *.cpp)
SRCS+=$(wildcard *.c)

# if you don't want some files to be compiled, add them here
REJECT_SRCS= # example: a.cpp b.cpp

# if you put your srcs in subfold, use below:
#vpath %.cpp ./subfold1
#vpath %.cpp ./subfold2

# some options
# example: MACRO1 MACRO2
DEFINES=

# example: ../test_lib ../test_lib2
INCLUDE_PATH=

# exampel: ../test_lib ../test_lib2
LIBRARY_PATH=

# example: ../test_lib/test.a ../test_lib/test2.a
STATIC_LIB= 

# example: pthread curl
DYNAMIC_LIB=pthread stdc++

#compile for 32-bit
ARG_m32=#-m32 

#-v verbose
ARG_v=

#for gdb
ARG_g=-g

#-O2 #optimization
ARG_O=

#c++ feature
STD=c++17

# if you have some other directories need to be compile before this directory,
# add them here for example: PRE_MAKE_DIRS=../a_lib_proj
# the 'a_lib_proj' directory's Makefile will be run before this directory
PRE_MAKE_DIRS= # example: ../test_lib


# *----------------------NO NEED TO MODIFY BELOW---------------------------*
ARCH=$(shell uname -s | awk '{print toupper($0)}')
ARG_DEFINES=$(addprefix -D,$(DEFINES) $(ARCH))
ARG_INCLUDE_PATH=$(addprefix -I,$(INCLUDE_PATH))
ARG_LIBRARY_PATH=$(addprefix -L,$(LIBRARY_PATH))
ARG_LLIB=$(addprefix -l,$(DYNAMIC_LIB)) $(STATIC_LIB)

CFLAGS=$(ARG_g) $(ARG_O) $(ARG_v) $(ARG_m32) $(ARG_DEFINES) $(ARG_INCLUDE_PATH)
CXXFLAGS=$(ARG_g) $(ARG_O) $(ARG_v) $(ARG_m32) $(ARG_DEFINES) $(ARG_INCLUDE_PATH) -std=$(STD)
LDFLAGS=$(ARG_v) -std=$(STD) $(ARG_LIBRARY_PATH)
LDFLAGS+= $(ARG_LLIB)

ALL_OBJS=$(addsuffix .o,$(basename $(SRCS)))
REJECT_OBJS=$(addsuffix .o,$(basename $(REJECT_SRCS)))
OBJS=$(notdir $(filter-out $(REJECT_OBJS),$(ALL_OBJS)))
DIR_OBJS=objs
vpath %.o $(DIR_OBJS)

DIR_DEPS=deps
DEPS=$(patsubst %.o,%.d,$(OBJS))

PWD=$(shell pwd)

ifdef PRE_MAKE_DIRS
all : pre_make change_dir $(OUT)
else
all : change_dir $(OUT)
endif

$(OUT) : $(OBJS)
ifeq ($(suffix $(OUT)),.a)
	ar -rc $(OUT) $(patsubst %.o,./$(DIR_OBJS)/%.o,$(OBJS))
else
ifeq ($(suffix $(OUT)),.so)
	$(LD) -shared -fPIC $(ARG_m32) $(patsubst %.o,./$(DIR_OBJS)/%.o,$(OBJS)) $(LDFLAGS) -o $(OUT)
else
	$(LD) $(ARG_m32) $(patsubst %.o,./$(DIR_OBJS)/%.o,$(OBJS)) $(LDFLAGS) -o $(OUT)
endif
endif

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o ./$(DIR_OBJS)/$@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o ./$(DIR_OBJS)/$@

$(DIR_DEPS)/%.d : %.cpp
	@[ -d $(DIR_DEPS) ] || mkdir -p $(DIR_DEPS)
	$(CXX) $(ARG_INCLUDE_PATH) -MM $< -o $@

$(DIR_DEPS)/%.d : %.c
	@[ -d $(DIR_DEPS) ] || mkdir -p $(DIR_DEPS)
	$(CC) $(ARG_INCLUDE_PATH) -MM $< -o $@

-include $(addprefix $(DIR_DEPS)/,$(DEPS))

pre_make:
	@for d in $(PRE_MAKE_DIRS); do \
	  if test -d "$$d"; then \
	    cd $$d && $(MAKE) $(MAKECMDGOALS); \
		cd $(PWD); \
	  fi; \
	done;

change_dir:
	@[ -d $(DIR_OBJS) ] || mkdir -p $(DIR_OBJS)

ifdef PRE_MAKE_DIRS
clean : pre_make
else
clean :
endif
	rm -rf $(DIR_OBJS) $(OUT) $(DIR_DEPS)
	@echo all cleaned

rebuild : clean all

.PHONY : all clean rebuild