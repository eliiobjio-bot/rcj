set(TARGET_NAME cy_server)
set(SOURCES cy_multilink.cpp
            cy_tcpserver.cpp
            cy_tcpserverfunc.cpp
            cy_vxi11srvfunc.cpp
            cy_vxi11svc.cpp
            cy_vxi11xdr.cpp
            cy_server.cpp)

#include_directories(
#    "/usr/include/tirpc"
#)

include_directories(${PROJECT_SOURCE_DIR}/lib/include/srsran/common)
include_directories(${PROJECT_SOURCE_DIR}/lib/include/srsran/server)
add_library(${TARGET_NAME} STATIC ${SOURCES})
if(RPCLib_FOUND)
  message("target link rpc lib successfully.")
  #include_directories(${RPCLIB_INCLUDE_DIR})
  #link_directories(${RPCLIB_LIBS})
  if(BUILD_UBUNTU22)
    target_link_libraries(${TARGET_NAME} tirpc)
  endif(BUILD_UBUNTU22)
endif(RPCLib_FOUND)

install(TARGETS ${TARGET_NAME} DESTINATION ${LIBRARY_DIR} OPTIONAL)