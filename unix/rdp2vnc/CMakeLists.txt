include_directories(${CMAKE_SOURCE_DIR}/unix/common)
include_directories(${CMAKE_SOURCE_DIR}/unix)
include_directories(${CMAKE_SOURCE_DIR}/common)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PACKAGES REQUIRED freerdp2 freerdp-client2 winpr2 xkbcommon)

add_executable(rdp2vnc
  buildtime.c
  rdp2vnc.cxx
  Geometry.cxx
  RDPPixelBuffer.cxx
  RDPDesktop.cxx
  RDPClient.cxx
)

target_link_libraries(rdp2vnc tx rfb network rdr unixcommon)

target_link_libraries(rdp2vnc ${PACKAGES_LIBRARIES})
target_include_directories(rdp2vnc PRIVATE ${PACKAGES_INCLUDE_DIRS})
target_compile_options(rdp2vnc PUBLIC ${PACKAGES_CFLAGS_OTHER})

install(TARGETS rdp2vnc DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
