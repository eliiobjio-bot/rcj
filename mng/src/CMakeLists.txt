
include_directories(${PROJECT_SOURCE_DIR}/lib/include/srsran/common)
include_directories(${PROJECT_SOURCE_DIR}/lib/include/srsran/server)
include_directories(${PROJECT_SOURCE_DIR}/lib/include/srsran/scpi)
include_directories(${PROJECT_SOURCE_DIR}/lib/include/srsran/net)
include_directories(${PROJECT_SOURCE_DIR}/lib/src/srslog)
include_directories(${PROJECT_SOURCE_DIR}/mng/hdr)
include_directories(
    ${Boost_INCLUDE_DIRS}
)
set(TARGET_NAME xwmng)
# Link libstdc++ and libgcc
if(BUILD_STATIC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif(BUILD_STATIC)

if (RPATH)
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif (RPATH)

# add_library(enb_cfg_parser STATIC parser.cc enb_cfg_parser.cc)
# target_link_libraries(enb_cfg_parser srsran_common ${LIBCONFIGPP_LIBRARIES})

# set(CY_SCPI_SOURCES cy_satscpiparse.cc cy_scpi_channel.cc)
add_executable(${TARGET_NAME} main.cc 
    control.cc 
    utils.cc
    init.cc
    server.cc
    mng_scpi_channel.cc
    mng_scpiparse.cc
    inspect.cc)


# set(SRSENB_SOURCES srsenb_phy srsenb_stack srsenb_common srsenb_s1ap srscnw srsenb_upper srsenb_mac srsenb_rrc srslog system)
# set(SRSRAN_SOURCES srsran_common srsran_mac srsran_phy srsran_gtpu srsran_rlc srsran_pdcp srsran_radio rrc_asn1 s1ap_asn1 enb_cfg_parser srslog support system)

# set(SRSENB_SOURCES ${SRSENB_SOURCES} srsgnb_stack srsgnb_ngap srsgnb_upper srsgnb_mac srsgnb_rrc cy_server)
# set(SRSRAN_SOURCES ${SRSRAN_SOURCES} rrc_nr_asn1 ngap_nr_asn1)

target_link_libraries(${TARGET_NAME}  ${Boost_LIBRARIES} srslog srsran_common cy_server config++)

if (RPATH)
  set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH ".")
endif (RPATH)

install(TARGETS ${TARGET_NAME} DESTINATION ${RUNTIME_DIR})
