INSTALLS += target
target.path = /lib

TEMPLATE = lib
CONFIG += warn_on dll
QT += sql

include(../modules_config.pri)

# Include projects module for DatabaseHandler
INCLUDEPATH += ../projects
LIBS += -L../projects -lprojects

SOURCES += communicationmanager.cpp

HEADERS += communicationmanager.h

