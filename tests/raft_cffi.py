import cffi
import subprocess

ffi = cffi.FFI()
ffi.set_source(
    "tests",
    """
    """,
    sources="""
        src/raft_log.c
        src/raft_server.c
        src/raft_server_properties.c
        src/raft_node.c
        """.split(),
    include_dirs=["include"],
    )
library = ffi.compile()

ffi = cffi.FFI()
lib = ffi.dlopen(library)


def load(fname):
    return '\n'.join(
        [line for line in subprocess.check_output(
            ["gcc", "-E", fname]).decode('utf-8').split('\n')])


ffi.cdef('void *malloc(size_t __size);')
ffi.cdef(load('include/raft.h'))
ffi.cdef(load('include/raft_log.h'))
