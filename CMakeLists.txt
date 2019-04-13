cmake_minimum_required (VERSION 2.6)
project(file_xfer)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

include_directories(src src/utils libs/slay2)

add_executable(fx_server
   server.cpp
   src/file_xfer_server.cpp
   src/utils/dirutils.cpp
   src/utils/dirutils_linux.cpp
   libs/slay2/crc32.c
   libs/slay2/slay2_buffer.cpp
   libs/slay2/slay2_scheduler.cpp
   libs/slay2/slay2.cpp
   libs/slay2/slay2_linux.cpp
)
target_link_libraries(fx_server pthread)



add_executable(fx_client
   client.cpp
   src/file_xfer_client.cpp
   libs/slay2/crc32.c
   libs/slay2/slay2_buffer.cpp
   libs/slay2/slay2_scheduler.cpp
   libs/slay2/slay2.cpp
   libs/slay2/slay2_linux.cpp
)
target_link_libraries(fx_client pthread)
