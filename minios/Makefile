######################################################################
# Mini-OS Definitions
######################################################################
BUILD                  ?= build
STUBDOM_NAME            = minipython
STUBDOM_ROOT            = $(realpath .)
ARCH                    := x86_64
XEN_TARGET_ARCH         := $(ARCH)
XEN_COMPILE_ARCH        := $(ARCH)
XEN_ROOT                ?= $(realpath ../../xen)
TOOLCHAIN_ROOT          ?= $(realpath ../../toolchain)
MINIOS_ROOT             ?= $(realpath ../../mini-os)
NEWLIB_ROOT             ?= $(TOOLCHAIN_ROOT)/$(ARCH)-root/x86_64-xen-elf
LWIP_ROOT               ?= $(TOOLCHAIN_ROOT)/$(ARCH)-root/x86_64-xen-elf
GCC_VERSION             ?= 4.9
verbose                 ?=
debug                   ?= y
CC                       = gcc
CPP                      = gcc
SED                      = sed
ECHO                     = @echo
PYTHON                   = python

# console
CONFIG_CONSFRONT_SYNC             ?= y

# vif
CONFIG_NETFRONT                    = y
CONFIG_NETFRONT_PERSISTENT_GRANTS ?= y
CONFIG_NETFRONT_GSO               ?= y
CONFIG_NETFRONT_POLL               = n
CONFIG_NETFRONT_POLLTIMEOUT        = 1
CONFIG_NETFRONT_WAITFORTX         ?= y
CONFIG_NETFRONT_LWIP_ONLY         ?= y

# lwip
CONFIG_LWIP                        = y
CONFIG_LWIP_MINIMAL                = y
CONFIG_LWIP_NOTHREADS              = y
CONFIG_LWIP_HEAP_ONLY             ?= n
CONFIG_LWIP_POOLS_ONLY             = n
CONFIG_START_NETWORK               = n
CONFIG_MP_LWIP_DEBUG               = n

# shfs (only if you know what you're doing!)
CONFIG_SHFS                        = n

include mkenv_minios.mk

######################################################################
# Objects and Flags
######################################################################
STUB_CFLAGS      += -Iinc                          \
                    -I.                            \
                    -I..                           \
                    -I../lib/mp-readline           \
                    -I../stmhal                    \
                    -I$(BUILD)                     \
		    -I../lib/netutils              \
                    -I../lib/timeutils             \
		    -DFATFS_ENABLE                 \

ifeq ($(CONFIG_SHFS),y)
STUB_CFLAGS      += -DSHFS_OPENBYNAME              \
                    -DSHFS_CACHE_READAHEAD=8       \
	            -DSHFS_CACHE_GROW              \
	            -DSHFS_ENABLE
endif

ifeq ($(CONFIG_MP_LWIP_DEBUG),y)
STUB_CFLAGS      += -DLWIP_IF_DEBUG                \
		    -DLWIP_SERVICE_DEBUG           \
		    -DLWIP_IP_DEBUG                \
		    -DLWIP_SYS_DEBUG              
endif

STUB_APP_OBJS0   := main.o                         \
		    minipython.o                   \
		    unix_mphal.o                   \
		    alloc.o                        \
		    builtin_open.o                 \
		    gccollect.o                    \
                    ../lib/utils/printf.o          \
                    ../lib/utils/pyexec.o          \
	            ../lib/timeutils/timeutils.o   \
                    ../lib/libc/string0.o          \
                    ../lib/mp-readline/readline.o  \
                    _frozen_mpy.o                  \
		    fatfs_port.o                   \
		    blkdev.o                       \
		    diskio.o                       \
		    mempool.o                      \
                    ring.o                         \
		    hexdump.o                      \
	            debug.o


ifeq ($(CONFIG_SHFS),y)
STUB_APP_OBJS0   += blkdev.o                       \
		    debug.o                        \
                    mempool.o                      \
                    ring.o                         \
                    shfs/htable.o                  \
                    shfs/shfs.o                    \
                    shfs/shfs_cache.o              \
                    shfs/shfs_check.o              \
                    shfs/shfs_fio.o    
endif

ifeq ($(CONFIG_LWIP),y)
STUB_APP_OBJS0   += mods/modlwip.o                 \
                    ../lib/netutils/netutils.o
endif

STUB_APP_OBJS0   += $(addprefix ../py/, $(PY_O_BASENAME))

STUB_APP_OBJS0   += $(addprefix mods/,\
                      modusocket.o    \
		      modtime.o       \
		      modos.o         \
                      )

STUB_BUILD_DIRS	 += $(STUBDOM_BUILD_DIR)/lib/utils        \
                    $(STUBDOM_BUILD_DIR)/lib/libc         \
                    $(STUBDOM_BUILD_DIR)/lib/mp-readline  \
                    $(STUBDOM_BUILD_DIR)/build            \
                    $(STUBDOM_BUILD_DIR)/py               \
                    $(STUBDOM_BUILD_DIR)/extmod           \
                    $(STUBDOM_BUILD_DIR)/lib/netutils     \
                    $(STUBDOM_BUILD_DIR)/lib/timeutils    \
                    $(STUBDOM_BUILD_DIR)/minipython/mods \
		    $(STUBDOM_BUILD_DIR)/lib/fatfs        \
		    $(STUBDOM_BUILD_DIR)/lib/fatfs/option


ifeq ($(CONFIG_SHFS),y)
STUB_BUILD_DIRS  +=  $(STUBDOM_BUILD_DIR)/$(STUBDOM_NAME)/shfs
endif

# MinOS' Makefile
include $(MINIOS_ROOT)/stub.mk

CFLAGS           += -std=gnu11


######################################################################
# Targets
######################################################################
# qstr definitions (must come before including py.mk)
QSTR_GEN_EXTRA_CFLAGS += -D__QSTR_EXTRACT -DN_X64 -DN_X86 -DN_THUMB -DN_ARM
QSTR_GEN_EXTRA_CFLAGS += -I$(BUILD)/tmp
MICROPY_FORCE_32BIT = 0
QSTR_DEFS = qstrdefsport.h
MPCONFIGPORT_MK = $(wildcard mpconfigport.mk)

$(STUB_APP_OBJS): | $(HEADER_BUILD)/qstrdefs.generated.h $(HEADER_BUILD)/mpversion.h

$(HEADER_BUILD)/qstrdefs.generated.h: $(PY_QSTR_DEFS) $(QSTR_DEFS) $(QSTR_DEFS_COLLECTED) $(PY_SRC)/makeqstrdata.py mpconfigport.h $(MPCONFIGPORT_MK) $(PY_SRC)/mpconfig.h | $(HEADER_BUILD)
	$(ECHO) "GEN $@"
	$(Q)cat $(PY_QSTR_DEFS) $(QSTR_DEFS) $(QSTR_DEFS_COLLECTED) | $(SED) 's/^Q(.*)/"&"/' | $(COMPILE) -E $(CFLAGS) - | sed 's/^"\(Q(.*)\)"/\1/' > $(HEADER_BUILD)/qstrdefs.preprocessed.h
	$(Q)$(PYTHON) $(PY_SRC)/makeqstrdata.py $(HEADER_BUILD)/qstrdefs.preprocessed.h > $@


$(HEADER_BUILD)/qstr.i.last: $(SRC_QSTR) | $(HEADER_BUILD)/mpversion.h
	$(ECHO) "GEN $@"
	if [ "$?" = "" ]; then \
	echo "QSTR Looks like -B used, trying to emulate"; \
	    $(COMPILE) -E $(QSTR_GEN_EXTRA_CFLAGS) $(CFLAGS) $^ >$(HEADER_BUILD)/qstr.i.last; \
	else \
	    $(COMPILE) -E $(QSTR_GEN_EXTRA_CFLAGS) $(CFLAGS) $? >$(HEADER_BUILD)/qstr.i.last; \
	fi

$(HEADER_BUILD)/qstr.split: $(HEADER_BUILD)/qstr.i.last
	$(ECHO) "GEN $@"
	$(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py split $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstr $(QSTR_DEFS_COLLECTED)
	$(Q)touch $@

$(QSTR_DEFS_COLLECTED): $(HEADER_BUILD)/qstr.split
	$(ECHO) "GEN $@"
	$(Q)$(PYTHON) $(PY_SRC)/makeqstrdefs.py cat $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstr $(QSTR_DEFS_COLLECTED)

$(STUB_APP_OBJ_DIR)/../py/emitnx64.o: CFLAGS += -DN_X64
$(STUB_APP_OBJ_DIR)/../py/emitnx64.o: $(STUB_APP_SRC_DIR)/../py/emitnative.c | build-reqs
	$(call ccompile, $(STUB_APP_INCLUDES) -c $< -o $@,'CC ')

$(STUB_APP_OBJ_DIR)/../py/emitnx86.o: CFLAGS += -DN_X86
$(STUB_APP_OBJ_DIR)/../py/emitnx86.o: $(STUB_APP_SRC_DIR)/../py/emitnative.c | build-reqs
	$(call ccompile, $(STUB_APP_INCLUDES) -c $< -o $@,'CC ')

$(STUB_APP_OBJ_DIR)/../py/emitnthumb.o: CFLAGS += -DN_THUMB
$(STUB_APP_OBJ_DIR)/../py/emitnthumb.o: $(STUB_APP_SRC_DIR)/../py/emitnative.c | build-reqs
	$(call ccompile, $(STUB_APP_INCLUDES) -c $< -o $@,'CC ')

$(STUB_APP_OBJ_DIR)/../py/emitnarm.o: CFLAGS += -DN_ARM
$(STUB_APP_OBJ_DIR)/../py/emitnarm.o: $(STUB_APP_SRC_DIR)/../py/emitnative.c | build-reqs
	$(call ccompile, $(STUB_APP_INCLUDES) -c $< -o $@,'CC ')

$(HEADER_BUILD):
	$(MKDIR) -p $@

FORCE:
.PHONY: FORCE
$(HEADER_BUILD)/mpversion.h: FORCE | $(HEADER_BUILD)
	$(Q)$(PYTHON) $(PY_SRC)/makeversionhdr.py $@

$(STUB_APP_SRC_DIR)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h 
	$(ECHO) "MISC freezing bytecode"
	$(Q)../tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

distclean: distclean_local
distclean_local:
	rm -f $(STUB_APP_SRC_DIR)/_frozen_mpy.c

printvars:
	@$(foreach V, \
		$(sort $(if $(VARS),$(filter $(VARS),$(.VARIABLES)),$(.VARIABLES))), \
		$(if $(filter-out environment% default automatic, \
				$(origin $V)), \
		$(info $V=$($V) ($(value $V)))))
