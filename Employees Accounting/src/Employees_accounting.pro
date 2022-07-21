QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dialoginsertinfo.cpp \
    dialogselectbackupversion.cpp \
    dialogselectorg.cpp \
    main.cpp \
    mainwindow.cpp \
    tablecommands.cpp \
    tcpclient.cpp

HEADERS += \
    databasecreation.h \
    dialoginsertinfo.h \
    dialogselectbackupversion.h \
    dialogselectorg.h \
    mainwindow.h \
    tablecommands.h \
    tcpclient.h \
    tcpdatatypes.h

FORMS += \
    dialoginsertinfo.ui \
    dialogselectbackupversion.ui \
    dialogselectorg.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

RC_ICONS = app_icon.ico
