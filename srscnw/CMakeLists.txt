########################################################################
# Setup the include and linker paths
########################################################################
if (ENABLE_SRSCNW)
  find_package(LibConfig REQUIRED)
  find_package(SCTP REQUIRED)

  if(BUILD_STATIC)
    set(LIBCONFIGPP_LIBRARIES "${LIBCONFIGPP_STATIC_LIBRARY_PATH}")
  endif(BUILD_STATIC)

  if(NOT Boost_FOUND)
      message(FATAL_ERROR "Boost required to compile srsCNW")
  endif()
  include_directories(
    ${Boost_INCLUDE_DIRS}
    ${SEC_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR})

  link_directories(
      ${Boost_LIBRARY_DIRS}
      ${SEC_LIBRARY_DIRS})
endif (ENABLE_SRSCNW)


########################################################################
# Create a library
########################################################################
aux_source_directory(${PROJECT_SOURCE_DIR}/srscnw/src SRC_LIST)

add_library(srscnw
    STATIC 
    ${SRC_LIST}
)

target_include_directories(srscnw
    PUBLIC 
    ${PROJECT_SOURCE_DIR}/srscnw/hdr
)

if (ENABLE_SRSCNW)
  if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
  endif(BUILD_STATIC)

  if (RPATH)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
  endif (RPATH)

  # Check if we need to link with librt for POSIX timer functions 6.17
  find_library(LIB_RT rt)
  if(LIB_RT)
    target_link_libraries(srscnw PRIVATE ${LIB_RT})
  endif()

endif (ENABLE_SRSCNW)

############################################################
# Create an executable
############################################################
if (ENABLE_SRSCNW)
  add_executable(srs5gc main.cc)
  target_link_libraries(srs5gc  srscnw
                                srsran_common
                                srslog
                                support
                                srsran_asn1
                                s1ap_asn1
                                srsran_gtpu
                                ${CMAKE_THREAD_LIBS_INIT}
                                ${Boost_LIBRARIES}
                                ${SEC_LIBRARIES}
                                ${LIBCONFIGPP_LIBRARIES}
                                ${SCTP_LIBRARIES})
  if (RPATH)
    set_target_properties(srs5gc PROPERTIES INSTALL_RPATH ".")
  endif (RPATH)
else (ENABLE_SRSCNW)
  message(STATUS "srsCNW-Dont generate elf file, srscnw build with srsenb")
endif (ENABLE_SRSCNW)

########################################################################
# Default configuration files
########################################################################
install(FILES epc.conf.example DESTINATION ${DATA_DIR})
install(FILES mbms.conf.example DESTINATION ${DATA_DIR})
install(FILES user_db.csv.example DESTINATION ${DATA_DIR})
install(PROGRAMS srsepc_if_masq.sh DESTINATION ${RUNTIME_DIR})