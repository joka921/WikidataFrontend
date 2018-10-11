cmake_minimum_required(VERSION 2.8.4)
project(WikidataFrontend C CXX)

# C/C++ Versions
set (CMAKE_C_STANDARD 11)
set (CMAKE_C_STANDARD_REQUIRED ON)
set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Check compiler versions:
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # Check version. If empty, warn. If too old, error out:
  if ("${CMAKE_CXX_COMPILER_VERSION}" STREQUAL "")
    message(WARNING "GCC Compiler version is unknown, proceed at your own risk!")
  elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
    message(FATAL_ERROR "GCC compiler version must be at least 4.8!")
  endif()
endif()

###############################################################################
##### Essential settings #####
###############################################################################

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall -Wextra -Wno-missing-field-initializers -Wno-ignored-qualifiers -Wno-reorder -Wno-unused-value -DGTEST_HAS_TR1_TUPLE=0 -DGTEST_USE_OWN_TR1_TUPLE=0")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

################################
# GTEST
################################
add_subdirectory(third_party/googletest/googletest)
include_directories(third_party/googletest/googletest/include)

################################
# NLOHNMANN-JSON 
################################
# Header only, nothing to include
include_directories(third_party/json/include/)


################################
# HTTP 
################################
add_subdirectory(third_party/httpclient-cpp)
include_directories(third_party/httpclient-cpp/HTTP)

find_package(CURL REQUIRED)

################################
# BOOST 
################################
find_package(Boost COMPONENTS serialization system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})


message(STATUS ---)
message(STATUS "CXX_FLAGS are : " ${CMAKE_CXX_FLAGS})
message(STATUS "CXX_FLAGS_RELEASE are : " ${CMAKE_CXX_FLAGS_RELEASE})
message(STATUS "CXX_FLAGS_DEBUG are : " ${CMAKE_CXX_FLAGS_DEBUG})
message(STATUS "IMPORTANT: Make sure you have selected the desired CMAKE_BUILD_TYPE")
message(STATUS ---)

###############################################################################
##### Actual project configuration #####
###############################################################################

add_subdirectory(src)
#add_subdirectory(src/experiments)
#add_subdirectory(test)

configure_file(src/web/search.html search.html)
configure_file(src/web/search.css search.css)
configure_file(src/web/search.js search.js)
configure_file(src/web/search2.js search2.js)
configure_file(src/web/search3.js search3.js)

configure_file(examples/Q42.entities examples/Q42.entities)
configure_file(examples/Q42.desc examples/Q42.desc)



add_executable(WikidataFrontendMain SearchServerMain.cpp)
target_link_libraries(WikidataFrontendMain SearchServer
  ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} httpclient ${CURL_LIBRARIES})

add_subdirectory(test)
enable_testing()
add_test(EntityFinderTest test/EntityFinderTest)
add_test(WikidataEntityTest test/WikidataEntityTest)


#enable_testing()
#add_test(SparqlParserTest test/SparqlParserTest)
