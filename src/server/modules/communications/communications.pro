unix {
    INSTALLS += target
    target.path = /lib
}

TEMPLATE = lib
CONFIG += warn_on dll
QT += sql

win32 {
    CONFIG -= dll
    CONFIG += staticlib
}

include(../modules_config.pri)

# Include projects module for DatabaseHandler
INCLUDEPATH += ../projects
unix {
    LIBS += -L../projects -lprojects
}

SOURCES += communicationmanager.cpp

HEADERS += communicationmanager.h

