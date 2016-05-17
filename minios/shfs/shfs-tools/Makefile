RM = rm -f
CC = gcc
LD = gcc
CFLAGS += -O3 -g -Wunused -Wtype-limits -D__SHFS_TOOLS__
LDFLAGS +=
LDLIBS += -luuid -lmhash

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%: %.o
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

shfs_mkfs: shfs_mkfs.o tools_common.o

shfs_admin: shfs_admin.o htable.o tools_common.o shfs_alloc.o shfs_check.o http_parser.o

all: shfs_mkfs shfs_admin

clean:
	$(RM) *.o core shfs_mkfs shfs_admin
