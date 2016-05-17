SHFS Tools
==========

The SHFS tools can be used to create and manage SHFS object stores.


Requirements
------------

In order to build the SHFS tools, you will need to have the following
shared libraries installed:
 * [libuuid](http://e2fsprogs.sourceforge.net)
 * [libmhash](http://mhash.sourceforge.net)

On Debian/Ubuntu you install them via:

    apt-get install libmhash2 libmhash-dev libuuid1 uuid-dev


Build Instructions
------------------

You build the SHFS tools with the following make command:

    make

Alternatively, you can also build a single tool by passing its executable
name (e.g., shfs_mkfs) to make:

    make shfs_mkfs


Examples: Using SHFS Tools
--------------------------

### Format a Physical Block Device with SHFS

    shfs_mkfs -n "NewVol#00" /dev/sdb2

### Create an 2GB SHFS Image File

    dd if=/dev/zero of=shfs-demo.img count=0 bs=1 seek=2G
    shfs_mkfs -n "SHFS-Demo" shfs-demo.img

### Adding Files to an SHFS Volume

    shfs_admin --add-obj /path/to/my_music.mp3 -m audio/mpeg3 shfs-demo.img

### Note

Please remember that you have to run remount on a MiniCache Domain after you
did changes to the object store while it is mounted.
