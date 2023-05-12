cmake_minimum_required(VERSION 3.9)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Sql Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Sql Network REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS forms)

set(UI_SOURCES
  forms/dialoginsertinfo.ui
  forms/dialogselectbackupversion.ui
  forms/dialogselectorg.ui
  forms/mainwindow.ui
)

set(HEADERS
  include/dialoginsertinfo.h
  include/dialogselectbackupversion.h
  include/dialogselectorg.h
  include/tablecommands.h
  include/tcpclient.h
  include/mainwindow.h
)

set(SOURCES
  src/dialoginsertinfo.cpp
  src/dialogselectbackupversion.cpp
  src/dialogselectorg.cpp
  src/tablecommands.cpp
  src/tcpclient.cpp
  src/mainwindow.cpp
  src/main.cpp
)

set(TARGET_NAME "app")

add_executable(${TARGET_NAME}
  resources/resources.qrc
  ${UI_SOURCES}
  ${HEADERS}
  ${SOURCES}
)

target_include_directories(${TARGET_NAME} PRIVATE
  include
)

target_link_libraries(${TARGET_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::Sql
  Qt${QT_VERSION_MAJOR}::Network
)

set_target_properties(${TARGET_NAME} PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTORCC ON
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON
)