INSTALLS += target
target.path = /lib

TEMPLATE = lib
CONFIG += warn_on dll

INCLUDEPATH += ../core ../packages ../modules/users

PRI_FILE = ../../../tupitube_config.pri
exists ($$PRI_FILE) {
    include ($$PRI_FILE)
} else {
    error("[base] Please, run configure first")
}

SOURCES += logger.cpp \
           observer.cpp \
           packagebase.cpp \
           settings.cpp

HEADERS += logger.h \
           observer.h \
           packagebase.h \
           settings.h
