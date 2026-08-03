unix {
    INSTALLS += target
    target.path = /lib
}

TEMPLATE = lib
CONFIG += warn_on dll
QT += sql

HEADERS += netproject.h \
           databasehandler.h \
           packagehandler.h \
           filemanager.h \
           projectmanager.h \
           projectrenderer.h \
           commandresultregistry.h

SOURCES += netproject.cpp \
           databasehandler.cpp \
           packagehandler.cpp \
           filemanager.cpp \
           projectmanager.cpp \
           projectrenderer.cpp \
           commandresultregistry.cpp

unix {
    INCLUDEPATH += /usr/local/quazip/include/quazip
    LIBS += -L/usr/local/quazip/lib -lquazip1-qt5
}
win32 {
    CONFIG -= dll
    CONFIG += staticlib
    include(../../../../quazip.win.pri)
}

LIB_DIR = ../../../lib
include($$LIB_DIR/lib.pri)

!include(../modules_config.pri){
    error("From module projects. File \"modules_config.pri\" can't be found")
}
