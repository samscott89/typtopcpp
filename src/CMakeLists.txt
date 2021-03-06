# find_path(PLOG_INCLUDE_DIRS include PATHS ${CMAKE_BINARY_DIR})
# GET_PROPERTY(CRYPTOPP_INCLUDE_DIRS GLOBAL PROPERTY CRYPTOPP_INCLUDE_DIRS)
# set(cryptopp_DIR ${CMAKE_BINARY_DIR}/cryptopp565/lib/cmake/cryptopp/)

# Setup protoc - there is no clean way to get the static library for protobuf, doing a hack
find_package(Protobuf REQUIRED)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS db.proto)
include_directories(${PROTOBUF_INCLUDE_DIRS})
get_filename_component(PROTO_LIB_DIR ${PROTOBUF_LIBRARY} DIRECTORY)
find_library(PROTOBUF_STATIC_LIB libprotobuf.a PROTO_LIB_DIR)

if(NOT PROTOBUF_STATIC_LIB)
  message(STATUS, "Protobuf static library not found. Using dynamic one.")
  find_library(PROTOBUF_STATIC_LIB libprotobuf.a PROTO_LIB_DIR)
endif()

find_package(CURL REQUIRED)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    SET(DEBUG 1)
endif()

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
        "${PROJECT_SOURCE_DIR}/typtopconfig.h.in"
        "${PROJECT_BINARY_DIR}/src/typtopconfig.h"
)

set(TYPTOP_SRCS typtop.cpp typtop.h ${PROTO_SRCS} ${PROTO_HDRS} ${PLOG_FILES} typtopconfig.h)

add_library(pw_crypto STATIC pw_crypto.cpp pw_crypto.h)
add_library(typtopdb STATIC ${TYPTOP_SRCS})
add_executable(typtop WIN32 typtop-main.cpp)
add_executable(testmain main.cpp)
set(CMAKE_MACOSX_RPATH 1)
add_library(b64decoder SHARED b64decoder.cpp)

# add_dependencies(pw_crypto ${CRYPTOPP_PREFIX})
add_dependencies(pw_crypto ${PLOG_PREFIX})
add_dependencies(typtopdb ${ZXCVBN_PREFIX})
add_dependencies(typtop ${ZXCVBN_PREFIX})

# Now the local dependencies
function (getListOfVarsStartingWith _prefix _varResult)
    get_cmake_property(_vars VARIABLES)
    string (REGEX MATCHALL "(^|;)${_prefix}[A-Za-z0-9_]*" _matchedVars "${_vars}")
    set (${_varResult} ${_matchedVars} PARENT_SCOPE)
endfunction()


# cryptopp CMake project so use that
# message(STATUS "${CMAKE_BINARY_DIR}/${CRYPTOPP_PREFIX}/lib/cmake/cryptopp")
find_package(cryptopp PATHS ${${CRYPTOPP_PREFIX}_BINARY_DIR}/lib/cmake/cryptopp NO_DEFAULT_PATH)

getListOfVarsStartingWith("cryptopp_" matchedVars)
foreach (_var IN LISTS matchedVars)
    message("${_var}=${${_var}}")
endforeach()
if (PLOG_INCLUDE_DIRS)
    message(STATUS "ADDING PLOG: ${PLOG_INCLUDE_DIRS} ${PLOG_FILES}")
    include_directories(${PLOG_INCLUDE_DIRS})
endif()


if (NOT ${${ZXCVBN_PREFIX}_SOURCE_DIR})
    set(ZXCVBN_INCLUDE_DIRS ${${ZXCVBN_PREFIX}_SOURCE_DIR})
    message(STATUS "ADDING ZXCVBN: ${ZXCVBN_INCLUDE_DIRS}")
    find_library(zxcvbn_lib libzxcvbn.a PATHS ${${ZXCVBN_PREFIX}_BINARY_DIR})
    if (zxcvbn_lib)
        set(extra_libs ${extra_libs} ${zxcvbn_lib})
#    else()
#        message(SEND_ERROR "zxcvbn not foud in ${${ZXCVBN_PREFIX}_BINARY_DIR} ${ZXCVBN_INCLUDE_DIRS} ${zxcvbn_lib}")
    endif ()

    message(STATUS "zxcvbn_lib: ${zxcvbn_lib}")
    # include_directories(${ZXCVBN_INCLUDE_DIRS})
else(ZXCVBN_INCLUDE_DIRS)
    message(SEND_ERROR "Zxcvbn not found")
endif()

check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC)
if(HAVE_LIBATOMIC)
    set(LIBATOMIC "-latomic")
endif()

target_include_directories (pw_crypto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CRYPTOPP_INCLUDE_DIRS})
target_link_libraries(pw_crypto cryptopp-static ${LIBATOMIC})


target_include_directories(typtopdb PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
        ${PLOG_INCLUDE_DIRS} ${ZXCVBN_INCLUDE_DIRS})
#target_include_directories(typtopdb PUBLIC )
#target_include_directories(typtopdb PUBLIC )

message(STATUS "PROTO_SRCS: ${PROTO_SRCS} ${PROTO_HDRS} ${CMAKE_CURRENT_BINARY_DIR} ${PROTOBUF_LIBRARIES}")
message(STATUS "CURL: ${CURL_LIBRARIES}")
target_link_libraries(typtopdb ${PROTOBUF_STATIC_LIB} pw_crypto ${extra_libs} ${CURL_LIBRARIES})


target_link_libraries(typtop pw_crypto typtopdb)

target_link_libraries(testmain pw_crypto typtopdb)

target_link_libraries(b64decoder pw_crypto)

add_subdirectory(pam_modules)
### --- INSTALL --- ###
# Install three things,
#   1> pam_typtop.so -> /usr/local/lib/security,
#   2> typtop -> /sbin/
#   3> typtop.conf -> /etc/pam.d
#   4> POST_INSTALL_SCRIPT := to update the pam_config files,

INSTALL(TARGETS typtop
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
        PERMISSIONS SETUID WORLD_EXECUTE OWNER_EXECUTE GROUP_EXECUTE
        OWNER_WRITE GROUP_READ WORLD_READ
        OWNER_READ GROUP_READ
        )
INSTALL(DIRECTORY typtop.d DESTINATION ${INSTALL_DATA_DIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION ${INSTALL_DATA_DIR}/typtop.d/)
