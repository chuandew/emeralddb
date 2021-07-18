cmake_minimum_required(VERSION 3.1.3 FATAL_ERROR)
project(emeralddb)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE DEBUG)

find_package(Boost 1.71 REQUIRED COMPONENTS
        system
        date_time
        filesystem
        iostreams
        thread
        program_options)
#message("Boost_FOUND is " ${Boost_FOUND}) #2020-01-15
#message("Boost_INCLUDE_DIR is " ${Boost_INCLUDE_DIR}) #2020-01-15
#message("Boost_DIR is " ${Boost_DIR}) #2020-01-15
#message("Boost_CONFIG is " ${Boost_CONFIG}) #2020-01-15
#message("Boost_VERSION is " ${Boost_VERSION}) #2020-01-15
#message("Boost_LIBRARIES is " ${Boost_LIBRARIES}) #2020-01-15

include_directories(${Boost_INCLUDE_DIR})
include_directories(bson/src)
include_directories(src/include)

list(APPEND bson_source
        bson/src/bsonobj.cpp
        bson/src/util/json.cpp
        bson/src/oid.cpp
        bson/src/lib/base64.cpp
        bson/src/lib/md5.cpp
        bson/src/lib/nonce.cpp)

add_executable(emeralddb
        src/pmd/pmdMain.cc
        src/pmd/pmdTcpListener.cc
        src/oss/ossSocket.cc
        ${bson_source})
target_link_libraries(emeralddb ${Boost_LIBRARIES})

add_executable(edb
        src/client/edb.cc
        src/client/command.cc
        src/client/commandFactory.cc
        src/oss/ossSocket.cc
        ${bson_source})
target_link_libraries(edb ${Boost_LIBRARIES})
