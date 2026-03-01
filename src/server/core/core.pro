INSTALLS += target 

target.path = /lib

TEMPLATE = lib
CONFIG += warn_on dll
QT += network sql

INCLUDEPATH += ../modules/users ../modules/backups ../modules/ban ../modules/communications

HEADERS += socketbase.h \
           connection.h \
           # packagehandler.h \
           packagehandlerbase.h \
           server.h \
           serverclient.h \
           defaultpackagehandler.h

SOURCES += socketbase.cpp \
           connection.cpp \
           # packagehandler.cpp \
           packagehandlerbase.cpp \
           server.cpp \
           serverclient.cpp \
           defaultpackagehandler.cpp

PRI_FILE = ../../../tupitube_config.pri
exists ($$PRI_FILE) {
    include ($$PRI_FILE)
} else {
    error("[core] Please, run configure first")
}

include(../server_config.pri)

TARGET = server
