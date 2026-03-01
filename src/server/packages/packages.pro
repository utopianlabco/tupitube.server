INSTALLS += target
target.path = /lib

TEMPLATE = lib
CONFIG += warn_on dll
TARGET = serverpackage

HEADERS += package.h \
           notice.h \
           notification.h \
           project.h \
           projectlist.h

SOURCES += package.cpp \
           notice.cpp \
           notification.cpp \
           project.cpp \
           projectlist.cpp

PRI_FILE = ../../../tupitube_config.pri
exists ($$PRI_FILE) {
    include ($$PRI_FILE)
} else {
    error("[packages] Please, run configure first")
}

include(../server_config.pri)
