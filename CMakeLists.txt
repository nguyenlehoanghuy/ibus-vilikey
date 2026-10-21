cmake_minimum_required(VERSION 3.16)
project(ibus-vilikey VERSION 0.2.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(ENABLE_IBUS  "Build the IBus engine" ON)
option(ENABLE_TESTS "Build the test suite"  ON)
set(VILIKEY_HOMEPAGE "" CACHE STRING "Project homepage, e.g. https://github.com/<user>/ibus-vilikey")

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)

set(VN_LIBEXECDIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/ibus-vilikey")
set(VN_DATADIR    "${CMAKE_INSTALL_FULL_DATADIR}/ibus-vilikey")
set(VN_BINDIR     "${CMAKE_INSTALL_FULL_BINDIR}")
set(VN_VERSION    "${PROJECT_VERSION}")

# ---- Engine gốc của UniKey (Phạm Kim Long) ---------------------------------
add_subdirectory(ukengine)

# ---- Lõi gõ tiếng Việt (không phụ thuộc IBus) ------------------------------
add_library(vn-core STATIC src/core.cpp)
target_include_directories(vn-core PUBLIC src)
target_link_libraries(vn-core PUBLIC ukengine PkgConfig::GLIB)
set_target_properties(vn-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(vn-core PRIVATE -Wall -Wextra)

# ---- Công cụ dòng lệnh -----------------------------------------------------
add_executable(vilikey-type tools/vilikey-type.cpp)          # thử gõ trên terminal
target_link_libraries(vilikey-type PRIVATE vn-core)

add_executable(vilikey-convert tools/vilikey-convert.cpp)    # chuyển mã (Toolkit)
target_link_libraries(vilikey-convert PRIVATE ukengine PkgConfig::GLIB)
target_compile_options(vilikey-convert PRIVATE -Wall -Wextra)

install(TARGETS vilikey-type vilikey-convert DESTINATION ${CMAKE_INSTALL_BINDIR})

# Cửa sổ chuyển mã (GTK4, Python)
configure_file(tools/vilikey-toolkit.py ${CMAKE_CURRENT_BINARY_DIR}/vilikey-toolkit @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/vilikey-toolkit DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/ibus-vilikey-toolkit.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# ---- IBus engine -----------------------------------------------------------
if(ENABLE_IBUS)
  pkg_check_modules(IBUS REQUIRED IMPORTED_TARGET ibus-1.0>=1.5.20)

  add_executable(ibus-engine-vilikey src/main.cpp src/engine.cpp src/settings.cpp)
  target_link_libraries(ibus-engine-vilikey PRIVATE vn-core PkgConfig::IBUS)
  target_compile_definitions(ibus-engine-vilikey PRIVATE
      VN_VERSION="${VN_VERSION}"
      VN_DATADIR="${VN_DATADIR}"
      VN_BINDIR="${VN_BINDIR}")
  target_compile_options(ibus-engine-vilikey PRIVATE -Wall -Wextra)

  configure_file(data/vilikey.xml.in ${CMAKE_CURRENT_BINARY_DIR}/vilikey.xml @ONLY)

  install(TARGETS ibus-engine-vilikey DESTINATION ${VN_LIBEXECDIR})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vilikey.xml
          DESTINATION ${CMAKE_INSTALL_DATADIR}/ibus/component)
  install(FILES data/icons/ibus-vilikey.svg DESTINATION ${VN_DATADIR}/icons)
  install(FILES data/icons/ibus-vilikey.svg
          DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
  install(FILES data/macro.txt DESTINATION ${VN_DATADIR})
endif()

# ---- Test ------------------------------------------------------------------
if(ENABLE_TESTS)
  enable_testing()
  add_executable(test-core tests/test_core.cpp)
  target_link_libraries(test-core PRIVATE vn-core)
  add_test(NAME core COMMAND test-core)
  add_test(NAME convert
           COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_convert.sh $<TARGET_FILE:vilikey-convert>)
endif()

# ---- Đóng gói .deb ---------------------------------------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "ibus-vilikey")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
# Người bảo trì: truyền khi build, ví dụ -DVILIKEY_MAINTAINER="Ten <email@example.com>"
set(VILIKEY_MAINTAINER "ViliKey contributors" CACHE STRING "Debian package maintainer")
set(CPACK_PACKAGE_CONTACT "${VILIKEY_MAINTAINER}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "ViliKey - Vietnamese input method for IBus (UniKey engine)")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
"ViliKey - Vietnamese input method for IBus (UniKey engine)
 ViliKey is a Vietnamese input method for Linux desktops using IBus,
 built on ukengine, the original UniKey engine by Pham Kim Long.
 It supports Telex, simple Telex, VNI and VIQR, spell checking, automatic
 restore of non-Vietnamese words, UniKey-compatible macros, preedit and
 direct editing modes, and a converter for legacy encodings
 (TCVN3, VNI, VISCII...) to Unicode.")
if(VILIKEY_HOMEPAGE)
  set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${VILIKEY_HOMEPAGE}")
endif()
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "ibus (>= 1.5.20)")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "python3-gi, gir1.2-gtk-4.0")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
include(CPack)
