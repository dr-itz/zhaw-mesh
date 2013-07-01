#
# Makefile for Meshy
# - For GNU Make
# - Supports GCC 3+ and Clang (automatically used on OS X)
# - Automatically generates make dependency rules for each .c file
#   (this means no manual dependency on any .h is necessary)
# - doesn't show commands except when called with V1=
#
# Tested on:
# - GNU Make 3.81
# - OS X 10.7 with Clang 4.1 and LLVM-GCC-4.2.1
# - Ubuntu 10.04 with GCC 4.4.3
# - Slowlaris 10 with GCC 3.4.6
#
# Copy-paste by Daniel Ritz, inspired by:
# - the "Git" Makefile
#
all::

# debugging, optimization
DBG = -ggdb
OPT = -Os
WARN = -Wall

# define the environment
CC = gcc
LD = gcc

###############################################################################
# Platform specific stuff, borrowed from Git Makefile
EXTLIBS =
ADDFLAGS =

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Linux)
	EXTLIBS += -lpthread
endif
ifeq ($(uname_S),Darwin)
	CC = clang
	LD = clang
endif
ifeq ($(uname_S),SunOS)
	EXTLIBS += -lsocket
	EXTLIBS += -lpthread
	EXTLIBS += -lnsl
endif

ifeq ($(CC),gcc)
	ADDFLAGS += -std=gnu99
endif

# make make shut up unless called with V=1
ifneq ($(findstring $(MAKEFLAGS),s),s)
ifndef V
	QUIET_CC = @echo '  CC  ' $@;
	QUIET_LD = @echo '  LD  ' $@;
endif
endif

###############################################################################
INCLUDES = -I.
CCFLAGS = $(OPT) $(WARN) $(DBG) -D__EXTENSIONS__ $(ADDFLAGS)
LDFLAGS = $(EXTLIBS)

TARGETS =
OBJS =

##############################################################################
# the program specific stuff

# generic stuff
LIB_DIR = lib
LIB_OBJ += $(LIB_DIR)/net.o
LIB_OBJ += $(LIB_DIR)/utils.o

OBJS += $(LIB_OBJ)


## mesh switch
MESHY_EXE = meshy
MESHY_OBJ += meshy.o
MESHY_OBJ += connection.o
MESHY_OBJ += receiver.o
MESHY_OBJ += packet.o
MESHY_OBJ += idcache.o
MESHY_OBJ += sendq.o
MESHY_OBJ += sender.o
MESHY_OBJ += routing.o

OBJS += $(MESHY_OBJ)
TARGETS += $(MESHY_EXE)

$(MESHY_EXE): $(MESHY_OBJ) $(LIB_OBJ)
	$(QUIET_LD)$(LD) $(LDFLAGS) $(LIBS) -o $@ $(MESHY_OBJ) $(LIB_OBJ)

## sendmsg
SENDMSG_EXE = sendmsg
SENDMSG_OBJ += sendmsg.o
SENDMSG_OBJ += packet.o

OBJS += $(SENDMSG_OBJ)
TARGETS += $(SENDMSG_EXE)

$(SENDMSG_EXE): $(SENDMSG_OBJ) $(LIB_OBJ)
	$(QUIET_LD)$(LD) $(LDFLAGS) $(LIBS) -o $@ $(SENDMSG_OBJ) $(LIB_OBJ)


###############################################################################
# the general targets
all:: $(TARGETS)

# automatic dependency generation, from Git Makefile
dep_files := $(foreach f,$(OBJS),$(dir $f).depend/$(notdir $f).d)
dep_dirs := $(addsuffix .depend,$(sort $(dir $(OBJS))))
$(dep_dirs):
	@mkdir -p $@

missing_dep_dirs := $(filter-out $(wildcard $(dep_dirs)),$(dep_dirs))
dep_file = $(dir $@).depend/$(notdir $@).d
dep_args = -MF $(dep_file) -MQ $@ -MMD -MP

# how we build C files, including dependeny files
%.o: %.c $(missing_dep_dirs)
	$(QUIET_CC)$(CC) $(dep_args) $(CCFLAGS) $(INCLUDES) -o $@ -c $<

# cleans all generated files
clean:
	@echo '  RM   all generated files'
	@rm -rf $(OBJS) $(TARGETS) $(dep_dirs)

# include the dependency files, from Git Makefile
dep_files_present := $(wildcard $(dep_files))
ifneq ($(dep_files_present),)
include $(dep_files_present)
endif
