
########################################################################
# Boost is required 
########################################################################
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost required to compile srsUE and ")
endif()

########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${SEC_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${SEC_LIBRARY_DIRS}
)


add_subdirectory(src)


install(FILES mng.conf.example DESTINATION ${DATA_DIR})