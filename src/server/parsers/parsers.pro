INSTALLS += target
target.path = /lib

HEADERS += chatparser.h \
           importprojectparser.h \
           listparser.h \
           listprojectsparser.h \
           newprojectparser.h \
           noticeparser.h \
           openprojectparser.h \
           projectactionparser.h \
           saveprojectparser.h \
           wallparser.h \
           projectimageparser.h \
           projectvideoparser.h \
           projectstoryboardparser.h \
           projectstoryboardpostparser.h \
           connectparser.h

SOURCES += chatparser.cpp \
           importprojectparser.cpp \
           listparser.cpp \
           listprojectsparser.cpp \
           newprojectparser.cpp \
           noticeparser.cpp \
           openprojectparser.cpp \
           projectactionparser.cpp \
           saveprojectparser.cpp \
           wallparser.cpp \
           projectimageparser.cpp \ 
           projectvideoparser.cpp \
           projectstoryboardparser.cpp \
           projectstoryboardpostparser.cpp \
           connectparser.cpp

#CONFIG += release warn_on staticlib
CONFIG += warn_on dll
TEMPLATE = lib

include(./parsers_config.pri)
