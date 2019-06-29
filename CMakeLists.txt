cmake_minimum_required(VERSION 2.6)
include(cmake/macros.cmake)

project(microplugins CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(microplugins_VERSION_MAJOR 2)
set(microplugins_VERSION_MINOR 0)

if(WIN32)
  set(CXXFLAGS -O2 -s -Wall -Wextra)
  set(LDLIBS pthread stdc++fs ole32 oleaut32 psapi advapi32)
else()
  set(CXXFLAGS -O2 -s -Wall -Wextra)
  set(LDLIBS dl pthread stdc++fs)
endif()

if(MAX_PLUGINS_ARGS)
  add_definitions(-DMAX_PLUGINS_ARGS=${MAX_PLUGINS_ARGS})
endif()

if(NDEBUG)
  add_definitions(-DNDEBUG)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/microplugins)

add_executable(microservice ${CMAKE_CURRENT_SOURCE_DIR}/examples/microservice.cxx)
target_compile_options(microservice PUBLIC ${CXXFLAGS})
target_link_libraries(microservice ${LDLIBS})

add_library(plugin1 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/examples/plugin1.cxx)
target_compile_options(plugin1 PUBLIC ${CXXFLAGS})
target_link_libraries(plugin1 ${LDLIBS})

add_library(bad_plugin1 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/examples/bad_plugin1.cxx)
target_compile_options(bad_plugin1 PUBLIC ${CXXFLAGS})
target_link_libraries(bad_plugin1 ${LDLIBS})

# https://habr.com/post/133512/
set(DOXY_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/microplugins)
set(DOXY_EXAMPLE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(DOXY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc)
set(DOXY_QUIET YES)
set(DOXY_WARN_LOGFILE ${CMAKE_CURRENT_BINARY_DIR}/Doxygen.log)
# set(DOXY_USE_MDFILE_AS_MAINPAGE README.md)
add_documentation(doc Doxyfile.in)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/microplugins DESTINATION include FILES_MATCHING PATTERN "*.hpp")

if(DOXYGEN_FOUND)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/html DESTINATION share/doc/microplugins)
endif(DOXYGEN_FOUND)
