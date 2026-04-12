INSTALLS += target
target.path = /lib

TEMPLATE = lib
CONFIG += warn_on dll
QT += sql

HEADERS += student.h \
           studentmanager.h \
           ban.h \
           ack.h

SOURCES += student.cpp \
           studentmanager.cpp \
           ban.cpp \
           ack.cpp

include(../modules_config.pri)
