# Mini-python

Mini-python is a Xen virtual machine. It consists of micro-python
running on top of the Xen paravirtualized MiniOS operating system.

## Building and running

First, edit the Makefile to set the correct paths to your Xen,
toolchain and MiniOS sources:

    XEN_ROOT                ?= $(realpath ../../[XENSRCS])
    TOOLCHAIN_ROOT          ?= $(realpath ../../[XENTOOLCHAIN)
    MINIOS_ROOT             ?= $(realpath ../../[MINIOS])

To build mini-python:

    $ make

To create the VM, first edit mini-python.xen and set the path to the
VM image:

    kernel = "/path/to/your/vm/image/mini-python_x86_64.gz"

Then create the VM with:

    $ xl create -c mini-python.xen



./mkwebfs ~/workspace/mini-python/minios/pythonsrcs/ pythonsrcs.shfs
/local/domain/0/backend/qdisk/166/51712
xenstore-ls

