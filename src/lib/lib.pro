INSTALLS += target
target.path = /lib

TARGET = tuputils
CONFIG += warn_on dll
TEMPLATE = lib

unix {
    INCLUDEPATH += /usr/local/ffmpeg/include
}
win32 {
    INCLUDEPATH += C:/ffmpeg/include
}

HEADERS = genericexportplugin.h \
          global.h

SOURCES = genericexportplugin.cpp

include(lib_config.pri)
