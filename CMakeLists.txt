cmake_minimum_required(VERSION 2.6)
project(microplugins CXX)

set_property(GLOBAL PROPERTY CXX_STANDARD 17)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)

include(cmake/macros.cmake)

set(microplugins_VERSION_MAJOR 1)
set(microplugins_VERSION_MINOR 0)

if(WIN32)
set(CXXFLAGS -Wall -Wextra -std=c++17)
set(LDLIBS pthread stdc++fs ole32 oleaut32 psapi advapi32)
else(WIN32)
set(CXXFLAGS -Wall -Wextra -std=c++17)
set(LDLIBS dl pthread stdc++fs)
endif(WIN32)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/microplugins)

add_executable(microservice ${CMAKE_CURRENT_SOURCE_DIR}/examples/microservice.cxx)
target_compile_options(microservice PUBLIC ${CXXFLAGS})
target_link_libraries(microservice ${LDLIBS})

add_library(plugin1 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/examples/plugin1.cxx)
target_compile_options(plugin1 PUBLIC ${CXXFLAGS})
target_link_libraries(plugin1 ${LDLIBS})

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
