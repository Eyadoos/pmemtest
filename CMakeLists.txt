cmake_minimum_required(VERSION 3.6.3)
project(ARTSynchronized)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_library(JemallocLib jemalloc)
find_package(Threads)
set(Tbb $ENV{TBBROOT}/lib/intel64/gcc4.7/libtbb.so)

include(FetchContent REQUIRED)
find_package(PkgConfig REQUIRED)
 
set(THIRD_PARTY_DIR "${PROJECT_BINARY_DIR}/_deps")
 
# Searching for PMDK ======================================================== #
message(STATUS "Searching for PMDK")
find_path(PMDK_INCLUDE_DIR libpmem.h)
pkg_check_modules(PMDK REQUIRED libpmemobj++>=1.5)
set(PMDK_INCLUDE_DIRS ${PMDK_INCLUDE_DIRS} ${PMDK_INCLUDE_DIR})
if(NOT PMDK_INCLUDE_DIRS OR "${PMDK_INCLUDE_DIRS}" STREQUAL "")
  message(FATAL_ERROR "ERROR: libpmem include directory not found.")
endif()
message(STATUS "  libpmem.h found in ${PMDK_INCLUDE_DIRS}")
mark_as_advanced(PMDK_LIBRARIES PMDK_INCLUDE_DIRS)

include_directories( $ENV{TBBROOT}/include ./Include ./ART/Include ./Lock/Include ./OptimisticLockCoupling/Include ./ROWEX/Include )

set(ART_FILES Lock/Tree.cpp OptimisticLockCoupling/Tree.cpp ROWEX/Tree.cpp ART/Tree.cpp)
add_library(ARTSynchronized ${ART_FILES}) 
target_link_libraries(ARTSynchronized ${Tbb} ${JemallocLib} ${CMAKE_THREAD_LIBS_INIT})


set(SOURCE_FILES example.cpp)
add_executable(example ${SOURCE_FILES})
target_link_libraries(example ARTSynchronized)
