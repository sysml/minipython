mods = ['_libc', '_markupbase', 'abc', 'argparse', 'asyncio_slow', 'base64', 'binascii', 'bisect', 'cgi', 'cmd', 'collections.defaultdict', 'collections.deque', 'collections', 'concurrent.futures', 'contextlib', 'copy', 'curses.ascii', 'email.charset', 'email.encoders', 'email.errors', 'email.feedparser', 'email.header', 'email.internal', 'email.message', 'email.parser', 'email.utils', 'errno', 'fcntl', 'ffilib', 'fnmatch', 'functools', 'getopt', 'glob', 'gzip', 'hashlib', 'heapq', 'hmac', 'html.entities', 'html.parser', 'html', 'http.client', 'inspect', 'io', 'itertools', 'json', 'keyword', 'locale', 'logging', 'machine', 'multiprocessing', 'operator', 'os.path', 'os', 'pickle', 'pkg_resources', 'pkgutil', 'pprint', 'pyb', 'pystone', 'pystone_lowmem', 'quopri', 'select', 'shutil', 'signal', 'socket', 'sqlite3', 'stat', 'string', 'struct', 'test.pystone', 'test.support', 'textwrap', 'time', 'timeit', 'traceback', 'tty', 'types', 'uasyncio.core', 'uasyncio.queues', 'uasyncio', 'ucontextlib', 'ucurses', 'unicodedata', 'unittest', 'upip', 'upysh', 'urequests', 'urllib.parse', 'urllib.urequest', 'utarfile', 'uu', 'warnings', 'weakref', 'xmltok']

WORKING = []
BROKEN = []
for m in mods:
    try:
        __import__(m)
        WORKING.append(m)
    except:
        BROKEN.append(m)

print("\nWorking modules:\n===========================\n" + str(WORKING))
print("\nBroken modules:\n===========================\n" + str(BROKEN))
