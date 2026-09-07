#
# FindRPCLib.cmake can be distributed with the source of
# your software that depends on rpc. This allows you
# to write
#
#     find_package(RPCLib)
#
# in your CMakeLists.txt.
#
# The script sets the following variables:
#
#   * RPCLIB_FOUND: true if the rpc headers and libraries were found
#   * RPCLIB_INCLUDE_DIR: the directory where rpc includes are located.
#                         This means #include "rpc/server.h" works if
#                         you add this directory, not #include "server.h".
#   * RPCLIB_LIBS: The static libraries of rpc (for now, this is only one
#                  library, but plural was chosen to be future-proof).
#   * RPCLIB_EXTRA_FLAGS: Extra flags that need to be added to the C++ compiler
#                         flags (e.g. CMAKE_CXX_FLAGS)
#   * RPCLIB_EXTRA_FLAGS_DEBUG: Same as above, but for debug configuration.
#                               (e.g. CMAKE_CXX_FLAGS_DEBUG)
#
# For finding in custom locations, you may set RPCLIB_ROOT as a cmake variable or
# environment variable.
#

set(FIND_RPCLIB_PATHS
    #${RPCLIB_ROOT}
    #$ENV{RPCLIB_ROOT}
    ${CMAKE_CURRENT_LIST_DIR}/..                   # To support in-tree build
    ${CMAKE_CURRENT_LIST_DIR}/../build/output/lib  #
    /usr/local
    /usr
    /usr/include
    /usr/include/tirpc
    /usr/lib/x86_64-linux-gnu
    /usr/lib/i386-linux-gnu
    /lib/x86_64-linux-gnu
    /usr/lib
    /usr/local/lib
    )
set(FIND_RPCBIND_PATHS
    /usr/sbin
    /usr/bin
    /usr
    /usr/local/bin
    )
find_path(RPCLIB_INCLUDE_DIR
    PATH_SUFFIXES "tirpc"
    NAMES "rpc/rpc.h"
    PATHS ${FIND_RPCLIB_PATHS})

find_program(FIND_RPCBIND
    NAMES "rpcbind"
    PATHS ${FIND_RPCLIB_PATHS})

if(FIND_RPCBIND)
    message("find the rpc bind exe!")
endif(FIND_RPCBIND)
find_library(RPCLIB_LIBS
    NAMES libtirpc.a libtirpc.so.3
    PATHS ${FIND_RPCBIND_PATHS}
)

message("get rpc libs: ${RPCLIB_LIBS}")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RPCLib
                      FOUND_VAR RPCLib_FOUND
                      REQUIRED_VARS RPCLIB_INCLUDE_DIR RPCLIB_LIBS FIND_RPCBIND)
