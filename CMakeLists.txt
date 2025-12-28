cmake_minimum_required(VERSION 3.26)

# --------------------------------------
# declare project based on info from vcpkg.json
# --------------------------------------
file(READ "vcpkg.json" VCPKG_JSON)
string(JSON VCPKG_PROJECT_NAME GET "${VCPKG_JSON}" "name")
string(JSON VCPKG_PROJECT_VERSION GET "${VCPKG_JSON}" "version")

project(${VCPKG_PROJECT_NAME}
  VERSION ${VCPKG_PROJECT_VERSION}
  LANGUAGES CXX
)

# --------------------------------------
# import helpers
# --------------------------------------
include(cmake/helpers.cmake)

# --------------------------------------
# setup base variables
# --------------------------------------
cmake_setup_default_project()

# --------------------------------------
# Qt / CMake automation
# --------------------------------------
cmake_setup_default_qt_project(
  AUTOUIC_SEARCH_PATHS "ui"
  MODULES Qt6Core Qt6Gui Qt6Widgets Qt6Multimedia Qt6SerialPort Qt6Charts
)

# --------------------------------------
# arduboy-manager target
# --------------------------------------
add_executable(xdm1041-gui
  res/app.rc

  include/xdm1041.h
  src/xdm1041.cpp

  include/MonitorWidget.h
  src/MonitorWidget.cpp
  src/MonitorWidget.ui

  src/main.cpp
)

target_include_directories(xdm1041-gui
  PRIVATE include
)

target_link_libraries(xdm1041-gui
  PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Multimedia
    Qt6::Charts
)

win32_disable_console(xdm1041-gui)

# install binaries & dependencies
install(TARGETS 
  xdm1041-gui
  BUNDLE DESTINATION .
)
win32_install_pdbs(xdm1041-gui)

# --------------------------------------
# install + Qt runtime deployment
# --------------------------------------
win32_deploy_qt(xdm1041-gui)
win32_install_global_qt_deps()
