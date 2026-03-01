INSTALLS += target
target.path = /lib

TEMPLATE = lib
CONFIG += warn_on dll
QT += sql

HEADERS += user.h \
           usermanager.h \
           ban.h \
           ack.h

SOURCES += user.cpp \
           usermanager.cpp \
           ban.cpp \
           ack.cpp

include(../modules_config.pri)
