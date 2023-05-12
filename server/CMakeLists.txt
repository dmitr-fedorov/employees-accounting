cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(HEADERS
  include/TcpServer.h
  include/TcpDataTypes.h
)

set(SOURCES
  src/TcpServer.cpp
  src/main.cpp
)

set(TARGET_NAME "server")

add_executable(${TARGET_NAME}
  ${HEADERS}
  ${SOURCES}
)

target_include_directories(${TARGET_NAME} PRIVATE
  include
)

target_link_libraries(${TARGET_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Network
)

set_target_properties(${TARGET_NAME} PROPERTIES
  AUTOMOC ON
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON
)