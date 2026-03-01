INSTALLS += target
target.path = /lib

TEMPLATE = lib
CONFIG += warn_on dll
QT += sql

HEADERS += netproject.h \
           databasehandler.h \
           packagehandler.h \
           filemanager.h \
           projectmanager.h

SOURCES += netproject.cpp \
           databasehandler.cpp \
           packagehandler.cpp \
           filemanager.cpp \
           projectmanager.cpp

INCLUDEPATH += /usr/local/quazip/include/quazip
LIBS += -L/usr/local/quazip/lib -lquazip1-qt5

!include(../modules_config.pri){
    error("From module projects. File \"modules_config.pri\" can't be found")
}
