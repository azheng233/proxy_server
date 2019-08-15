# CMakeLists for urlserver
# ypf@xdja.com 2018-03-20 11:04:47

cmake_minimum_required (VERSION 2.8)

project (urlserver C)

#aux_source_directory (src SRC_FILE)
file (GLOB_RECURSE SRC_FILE src/*.c)
include_directories (src)

option(BUILD_WITH_JIT "Enable JIT CARD." OFF)
option(BUILD_WITH_LICENSE "Enable License check." OFF)

if (BUILD_WITH_JIT)
    #list (APPEND SRC_FILE ${PROJECT_SOURCE_DIR}/src/jit_card.c)
    set (depends JIT_API)
else ()
    message("Build without JIT library")
    list (REMOVE_ITEM SRC_FILE ${PROJECT_SOURCE_DIR}/src/jit_card.c)
    set (DEFINITIONS NO_JIT)
endif (BUILD_WITH_JIT)

if (BUILD_WITH_LICENSE)
    set (depends ${depends} lcsvfy)
else ()
    message("Build without license library")
    set (DEFINITIONS ${DEFINITIONS} NO_LICENSE)
endif (BUILD_WITH_LICENSE)

set (DEPEND_LIBS ${depends} ssl crypto jansson)

include (cmake/feature.detect.cachesize.cmake)

set (DEFINITIONS ${DEFINITIONS} CACHE_LINESIZE=${CACHE_LINESIZE})

set_source_files_properties (
    ${PROJECT_SOURCE_DIR}/src/license.c
    PROPERTIES COMPILE_DEFINITIONS _XOPEN_SOURCE
    )

#set (DEFINITIONS ${DEFINITIONS} _DEBUG_NOSSL)
#set (DEFINITIONS ${DEFINITIONS} DEBUG_ALLOW_ALL_URL)
#set (DEFINITIONS ${DEFINITIONS} REQUEST_ALL_POLICY)
#set (DEFINITIONS ${DEFINITIONS} ENABLE_IPFILTE_MANAGER)
set (DEFINITIONS ${DEFINITIONS} _USE_SWHASH)
#set (DEFINITIONS ${DEFINITIONS} WITH_POLICY_TEST)

set (buildinfo_file ${PROJECT_BINARY_DIR}/build.c)
list (APPEND SRC_FILE ${buildinfo_file})
message (STATUS "Saving build info to ${buildinfo_file}")
file (WRITE ${buildinfo_file} "char build_info[] = \"${DEFINITIONS}\";")

#set (CMAKE_VERBOSE_MAKEFILE on)

include (cmake/common.execute.cmake)

install (
    FILES ${PROJECT_SOURCE_DIR}/src/url.conf
    DESTINATION etc
    )
