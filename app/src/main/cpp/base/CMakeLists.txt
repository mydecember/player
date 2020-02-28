
cmake_minimum_required(VERSION 3.4.1)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}  BASE_SRC_LIST)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_library( log-lib
            log )

find_library( android-lib
        android )

add_library(
             base
             STATIC
             ${BASE_SRC_LIST} )


