INSTALLS += target
target.path = /lib

TEMPLATE = lib
CONFIG += warn_on dll
QT += sql

include(../modules_config.pri)
SOURCES += communicationmanager.cpp

HEADERS += communicationmanager.h

