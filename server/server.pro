QT -= gui
QT += core network

CONFIG += c++11 console
CONFIG -= app_bundle

INCLUDEPATH += \
    include

SOURCES += \
        src/TcpServer.cpp \
        src/main.cpp

HEADERS += \
    include/TcpDataTypes.h \
    include/TcpServer.h
