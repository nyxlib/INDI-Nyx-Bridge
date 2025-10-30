#----------------------------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.14)

#----------------------------------------------------------------------------------------------------------------------

project(IndiNyxBridge
    VERSION 0.1.0
    LANGUAGES C CXX
)

#----------------------------------------------------------------------------------------------------------------------

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-D_GNU_SOURCE -DMG_ENABLE_CUSTOM_LOG=1 -DMG_ENABLE_DIRLIST=0 -DMG_ENABLE_POLL=0 -DMG_ENABLE_EPOLL=1 -DMG_ENABLE_SSI=0)

#----------------------------------------------------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

find_package(PkgConfig REQUIRED)

find_package(LibXml2 REQUIRED)

find_package(Threads REQUIRED)

find_package(INDI REQUIRED)

#----------------------------------------------------------------------------------------------------------------------

add_executable(indi_nyx
    src/indi_nyx_driver.cpp
    src/external/mongoose.c
    src/string_builder.c
    src/port_finder.c
    src/logger.cpp
    src/bridge.c
    src/transform_json_to_xml.c
    src/transform_xml_to_json.c
)

target_link_libraries(indi_nyx
    LibXml2::LibXml2
    Threads::Threads
    ${INDI_LIBRARIES}
)

#----------------------------------------------------------------------------------------------------------------------
