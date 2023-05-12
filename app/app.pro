QT       += core gui widgets sql network

CONFIG += c++11

INCLUDEPATH += \
    include

SOURCES += \
    src/dialoginsertinfo.cpp \
    src/dialogselectbackupversion.cpp \
    src/dialogselectorg.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/tablecommands.cpp \
    src/tcpclient.cpp

HEADERS += \
    include/databasecreation.h \
    include/dialoginsertinfo.h \
    include/dialogselectbackupversion.h \
    include/dialogselectorg.h \
    include/mainwindow.h \
    include/tablecommands.h \
    include/tcpclient.h \
    include/tcpdatatypes.h

FORMS += \
    forms/dialoginsertinfo.ui \
    forms/dialogselectbackupversion.ui \
    forms/dialogselectorg.ui \
    forms/mainwindow.ui

RESOURCES += \
    resources/resources.qrc

RC_ICONS = resources/icons/app_icon.ico
