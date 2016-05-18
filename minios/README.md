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

If you'll be using the network, make sure to first create a bridge with
brctl if you don't have one yet and edit the vif line in mini-python.xen:

      vif  = [ 'mac=00:11:22:33:44:55,bridge=vhostonly0' ]

If you're using the command do_file and SHFS, add a disk:

      disk = [ 'file:/root/workspace/mini-python/minios/minipython-volume.shfs,xvda,w']

To create minipython-volume.shfs, in the scripts subdirectory run:

    ./mkwebfs /path/to/your/python/files pythonsrcs.shfs

Get the signature of the file you want to run with:

    shfs/shfs-tools/shfs_admin -l ~/workspace/mini-python/minios/minipython-volume.shfs

(if shfs_admin isn't there run "make" in that directory first).

Set the default file flag with

    /shfs_admin -d [signature] minipython-volume.shfs

Finally, create the VM with:

    $ xl create -c mini-python.xen


