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

If you're using the command do_file and CONFIG_SHFS=y, add a disk:

      disk = [ 'file:/root/workspace/mini-python/minios/minipython-volume.shfs,xvda,w']

To create minipython-volume.shfs, in the scripts subdirectory run:

    ./mkwebfs /path/to/your/python/files pythonsrcs.shfs

Get the signature of the file you want to run with:

    shfs/shfs-tools/shfs_admin -l ~/workspace/mini-python/minios/minipython-volume.shfs

(if shfs_admin isn't there run "make" in that directory first).

Set the default file flag with

    /shfs_admin -d [signature] minipython-volume.shfs

If you're using a FAT volume (mini-python's standard FS) add a disk:

disk = [ 'file:/root/workspace/mini-python/minios/mini-python-demo-test.img,xvda,w']

OR

disk = ['phy:/dev/loop0,xvda,w']

Finally, create the VM with:

    $ xl create -c mini-python.xen


## MicroPython libs

To add https://github.com/micropython/micropython-lib to mini-python, edit
tools/libimport.py and set the MICROPY_LIB_DIR and MINIPYTHON_TARGET_DIR
variables, then

    python tools/libimport.py

The program will print which modules it actually copied (it ignores
placeholder libraries, i.e., those with empty .py files). As of June 2016
the output was:

Added libs:
=====================
['_libc', '_markupbase', 'abc', 'argparse', 'asyncio_slow', 'base64', 'binascii', 'bisect', 'cgi', 'cmd', 'collections.defaultdict', 'collections.deque', 'collections', 'concurrent.futures', 'contextlib', 'copy', 'curses.ascii', 'email.charset', 'email.encoders', 'email.errors', 'email.feedparser', 'email.header', 'email.internal', 'email.message', 'email.parser', 'email.utils', 'errno', 'fcntl', 'ffilib', 'fnmatch', 'functools', 'getopt', 'glob', 'gzip', 'hashlib', 'heapq', 'hmac', 'html.entities', 'html.parser', 'html', 'http.client', 'inspect', 'io', 'itertools', 'json', 'keyword', 'locale', 'logging', 'machine', 'multiprocessing', 'operator', 'os.path', 'os', 'pickle', 'pkg_resources', 'pkgutil', 'pprint', 'pyb', 'pystone', 'pystone_lowmem', 'quopri', 'select', 'shutil', 'signal', 'socket', 'sqlite3', 'stat', 'string', 'struct', 'test.pystone', 'test.support', 'textwrap', 'time', 'timeit', 'traceback', 'tty', 'types', 'uasyncio.core', 'uasyncio.queues', 'uasyncio', 'ucontextlib', 'ucurses', 'unicodedata', 'unittest', 'upip', 'upysh', 'urequests', 'urllib.parse', 'urllib.urequest', 'utarfile', 'uu', 'warnings', 'weakref', 'xmltok']


Ignored libs:
=====================
['binhex', 'calendar', 'csv', 'datetime', 'dbm', 'decimal', 'difflib', 'formatter', 'fractions', 'ftplib', 'getpass', 'gettext', 'imaplib', 'imp', 'ipaddress', 'mailbox', 'mailcap', 'mimetypes', 'nntplib', 'numbers', 'optparse', 'pathlib', 'pdb', 'pickletools', 'platform', 'poplib', 'posixpath', 'profile', 'pty', 'queue', 'random', 'reprlib', 'runpy', 'sched', 'selectors', 'shelve', 'shlex', 'smtplib', 'socketserver', 'statistics', 'stringprep', 'subprocess', 'tarfile', 'telnetlib', 'tempfile', 'threading', 'trace', 'urllib', 'uuid', 'zipfile']

The output under "Added libs" can be copied into examples/test_tryexcept.py to
see which modules will actually run under mini-python. Here's sample output
from that program:

Working modules:
===========================
['abc', 'argparse', 'binascii', 'bisect', 'cmd', 'collections', 'copy', 'errno', 'functools', 'getopt', 'gzip', 'hashlib', 'heapq', 'hmac', 'html', 'inspect', 'io', 'itertools', 'json', 'keyword', 'locale', 'machine', 'operator', 'os', 'pickle', 'pkgutil', 'pprint', 'pyb', 'quopri', 'select', 'shutil', 'socket', 'stat', 'string', 'struct', 'traceback', 'types', 'uasyncio', 'ucontextlib', 'ucurses', 'unicodedata', 'unittest', 'urequests', 'utarfile', 'uu', 'warnings', 'weakref', 'xmltok']

Broken modules:
===========================
['_libc', '_markupbase', 'asyncio_slow', 'base64', 'cgi', 'collections.defaultdict', 'collections.deque', 'concurrent.futures', 'contextlib', 'curses.ascii', 'email.charset', 'email.encoders', 'email.errors', 'email.feedparser', 'email.header', 'email.internal', 'email.message', 'email.parser', 'email.utils', 'fcntl', 'ffilib', 'fnmatch', 'glob', 'html.entities', 'html.parser', 'http.client', 'logging', 'multiprocessing', 'os.path', 'pkg_resources', 'pystone', 'pystone_lowmem', 'signal', 'sqlite3', 'test.pystone', 'test.support', 'textwrap', 'time', 'timeit', 'tty', 'uasyncio.core', 'uasyncio.queues', 'upip', 'upysh', 'urllib.parse', 'urllib.urequest']