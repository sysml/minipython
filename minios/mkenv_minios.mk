ifneq ($(lastword a b),b)
$(error These Makefiles require make 3.81 or newer)
endif

# Set TOP to be the path to get from the current directory (where make was
# invoked) to the top of the tree. $(lastword $(MAKEFILE_LIST)) returns
# the name of this makefile relative to where make was invoked.
#
# We assume that this file is in the py directory so we use $(dir ) twice
# to get to the top of the tree.

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
TOP := ..

#BUILD ?= build
PY_BUILD = $(BUILD)/py

# where autogenerated header files go
HEADER_BUILD = $(BUILD)/genhdr

# file containing qstr defs for the core Python bit
PY_QSTR_DEFS = $(PY_SRC)/qstrdefs.h

QSTR_DEFS_COLLECTED = $(HEADER_BUILD)/qstrdefs.collected.h

#SED = sed

# py object files
PY_O_BASENAME = \
	mpstate.o \
	nlrx86.o \
	nlrx64.o \
	nlrthumb.o \
	nlrxtensa.o \
	nlrsetjmp.o \
	malloc.o \
	gc.o \
	qstr.o \
	vstr.o \
	mpprint.o \
	unicode.o \
	mpz.o \
	lexer.o \
	lexerstr.o \
	parse.o \
	scope.o \
	compile.o \
	emitcommon.o \
	emitbc.o \
	asmx64.o \
	emitnx64.o \
	asmx86.o \
	emitnx86.o \
	asmthumb.o \
	emitnthumb.o \
	emitinlinethumb.o \
	asmarm.o \
	emitnarm.o \
	formatfloat.o \
	parsenumbase.o \
	parsenum.o \
	emitglue.o \
	runtime.o \
	runtime_utils.o \
	nativeglue.o \
	stackctrl.o \
	argcheck.o \
	warning.o \
	map.o \
	obj.o \
	objarray.o \
	objattrtuple.o \
	objbool.o \
	objboundmeth.o \
	objcell.o \
	objclosure.o \
	objcomplex.o \
	objdict.o \
	objenumerate.o \
	objexcept.o \
	objfilter.o \
	objfloat.o \
	objfun.o \
	objgenerator.o \
	objgetitemiter.o \
	objint.o \
	objint_longlong.o \
	objint_mpz.o \
	objlist.o \
	objmap.o \
	objmodule.o \
	objobject.o \
	objpolyiter.o \
	objproperty.o \
	objnone.o \
	objnamedtuple.o \
	objrange.o \
	objreversed.o \
	objset.o \
	objsingleton.o \
	objslice.o \
	objstr.o \
	objstrunicode.o \
	objstringio.o \
	objtuple.o \
	objtype.o \
	objzip.o \
	opmethods.o \
	sequence.o \
	stream.o \
	binary.o \
	builtinimport.o \
	builtinevex.o \
	modarray.o \
	modbuiltins.o \
	modcollections.o \
	modgc.o \
	modio.o \
	modmath.o \
	modcmath.o \
	modmicropython.o \
	modstruct.o \
	modsys.o \
	vm.o \
	bc.o \
	showbc.o \
	repl.o \
	smallint.o \
	frozenmod.o \
	../extmod/moductypes.o \
	../extmod/modujson.o \
	../extmod/modure.o \
	../extmod/moduzlib.o \
	../extmod/moduheapq.o \
	../extmod/moduhashlib.o \
	../extmod/modubinascii.o \
	../extmod/machine_mem.o \
	../extmod/machine_i2c.o \
	../extmod/modussl.o \
	../extmod/modurandom.o \
	../extmod/modwebsocket.o \
	../extmod/modwebrepl.o \
	../extmod/modframebuf.o \
	../extmod/fsusermount.o \
	../extmod/moduos_dupterm.o

ifeq ($(CONFIG_SHFS),n)
PY_O_BASENAME += ../extmod/vfs_fat.o \
	../extmod/vfs_fat_ffconf.o \
	../extmod/vfs_fat_file.o \
	../extmod/vfs_fat_lexer.o \
	../extmod/vfs_fat_misc.o \
        ../lib/fatfs/ff.o \
	../lib/fatfs/option/cc932.o	
endif

ifeq ($(CONFIG_LWIP),y)
LWIP_ROOT ?= $(realpath ../../toolchain)/x86_64-root/x86_64-xen-elf
SRC_MOD += ../extmod/modlwip.c ../lib/netutils/netutils.c
SRC_MOD += $(addprefix $(LWIP_ROOT)/src/lwip/,\
	core/def.c \
        core/dns.c \
        core/init.c \
	core/mem.c \
        core/memp.c \
        core/netif.c \
        core/pbuf.c \
        core/raw.c \
	core/stats.c \
        core/sys.c \
        core/tcp.c \
        core/tcp_in.c \
	core/tcp_out.c \
        core/timers.c \
        core/udp.c \
	core/ipv4/autoip.c \
        core/ipv4/icmp.c \
        core/ipv4/igmp.c \
        core/ipv4/ip4_addr.c \
        core/ipv4/ip4.c \
        core/ipv4/ip_frag.c \
        )
endif

# prepend the build destination prefix to the py object files
PY_O = $(addprefix $(PY_BUILD)/, $(PY_O_BASENAME))

# Sources that may contain qstrings
SRC_QSTR_IGNORE = nlr% emitnx% emitnthumb% emitnarm%
SRC_QSTR = $(SRC_MOD) $(addprefix ../py/,$(filter-out $(SRC_QSTR_IGNORE),$(PY_O_BASENAME:.o=.c)) emitnative.c)

PY_SRC ?= $(TOP)/py

#ECHO = @echo
#PYTHON = python

#MKENV_INCLUDED = 1
