# Minipython

Minipython is a Xen virtual machine able to run Python scripts. It consists of MicroPython (https://github.com/micropython/micropython) running on top of the Xen paravirtualized MiniOS operating system.

## Building

Clone the minipython repo, then:

	$ cd micropython
	$ git submodule init
	$ git submodule update

Next, edit minipython's Makefile (found in minios/Makefile) and set the paths to the Xen sources, the Xen toolchain sources and the MiniOS sources:

    XEN_ROOT                ?= $(realpath ../../[XENSRCS])
    TOOLCHAIN_ROOT          ?= $(realpath ../../[XENTOOLCHAIN)
    MINIOS_ROOT             ?= $(realpath ../../[MINIOS])

There are a number of options at the top of that Makefile that you can also set. For instance, by default, networking via lwip is enabled.

To build minipython:

    $ cd minios
    $ make

## Specifying a Python Script

To specify a Python script edit minios/main.c . You have two main options: do\_str(), which allows you to specify a string for your script directly in main.c, and do_file() which retrieves and executes a file from your filesystem; search for examples of these at the bottom of the main() function in main.c (and don't forget to make again if you modify the file).

## Running

The minipython VM can be simply run with

	$ xl create -c minipython.xen
	
Before this will work though we need to set up a filesystem and networking (the latter is only needed if the Makefile has networking enabled; by default it is). 

### Filesystem

Minipython uses FAT as its default filesystem type. Assuming your FAT volume is called minipython-fat.img, first mount it with:

	$ losetup /dev/loop0 minipython-fat.img
	$ kpartx -a /dev/loop0
	$ mkdir /mnt/fat
	$ mount /dev/mapper/loop0p1 /mnt/fat/

Finally specify a disk in minios/minipython.xen:


	disk = ['phy:/dev/loop0,xvda,w']


#### SHFS

[Use this only if you know what you're doing!]

First, set CONFIG_SHFS=y in minios/Makefile (make distclean; make if you had previously built the image). Then add a disk to minios/minipython.xen:

      disk = [ 'file:/path/to/your/volume/minipython-vol.shfs,xvda,w']

To create minipython-vol.shfs, in the minios/shfs/shfs-tools/ subdirectory run make, then:

    ./mkwebfs /path/to/your/python/files minipython-vol.shfs

Get the signature of the file you want to run with:

    ./shfs_admin -l minipython-vol.shfs

Set the default file flag with

    /shfs_admin -d [signature] minipython-vol.shfs



### Networking

First, create a bridge with brctl and bring it up:

	$ brctl addbr minipythonbr
	$ ifconfig minipythonbr 192.168.0.1 netmask 255.255.255.0 up

Then, add a vif to minios/minipython.xen: 

      vif  = [ 'mac=00:11:22:33:44:55,bridge=minipythonbr,ip=192.168.0.100' ]

To test it out you can run the HTTP server script provided with the minipython sources:

	$ cp minios/examples/http_server.py /mnt/fat

Add the line to main.c's main() function (before the call to mp_deinit()):

	do_file("http_server.py");
	
Run make and xl create -c minipython.xen. You can test that it is working by running the following command on the host (i.e., dom0):

	$ wget 192.168.0.100:8080 --no-proxy

## MicroPython Libs

To add https://github.com/micropython/micropython-lib to minipython, edit
minos/tools/libimport.py and set the MICROPY\_LIB\_DIR and MINIPYTHON\_TARGET\_DIR
variables, then

    $ python minios/tools/libimport.py

The program will print which modules it actually copied (it ignores
placeholder libraries, i.e., those with empty .py files). 

The output under "Added libs" (a Python array) can be copied into minios/examples/test_tryexcept.py to see which modules will actually run under minipython.