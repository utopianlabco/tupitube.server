# Set the plugins directory for tests
PLUGINS_PATH = /usr/local/tupitube.desk/lib/tupitube/plugins
DEFINES += TEST_PLUGINS_PATH=\\\"$$PLUGINS_PATH\\\"
DEFINES += TUP_DEBUG
DEFINES += TUPITUBE_TEST

SOURCES += \
	../src/server/modules/students/ack.cpp \
	../src/server/modules/students/ban.cpp \
	../src/server/base/settings.cpp
# Ensure all QObject/core implementations are linked
SOURCES += \
	../src/server/base/observer.cpp \
	../src/server/modules/students/studentmanager.cpp
QT += xml sql testlib
# QT += core gui svg xml network
SOURCES += \
	../src/server/base/packagebase.cpp \
	../src/server/modules/students/student.cpp
TEMPLATE = app
TARGET = tests
INCLUDEPATH += . ../src/server/packages ../src/server/core ../src/server/base ../src/server/modules/projects ../src/server/parsers ../src/server/modules/communications /usr/local/quazip/include/quazip ../src/lib
SOURCES += ../src/server/modules/communications/communicationmanager.cpp
SOURCES += \
	../src/server/parsers/chatparser.cpp \
	../src/server/parsers/connectparser.cpp \
	../src/server/parsers/importprojectparser.cpp \
	../src/server/parsers/listparser.cpp \
	../src/server/parsers/listprojectsparser.cpp \
	../src/server/parsers/newprojectparser.cpp \
	../src/server/parsers/noticeparser.cpp \
	../src/server/parsers/openprojectparser.cpp \
	../src/server/parsers/projectactionparser.cpp \
	../src/server/parsers/projectimageparser.cpp \
	../src/server/parsers/projectstoryboardparser.cpp \
	../src/server/parsers/projectstoryboardpostparser.cpp \
	../src/server/parsers/projectvideoparser.cpp \
	../src/server/parsers/saveprojectparser.cpp \
	../src/server/parsers/wallparser.cpp
include(../tupitube_config.pri)


# Input

SOURCES += ../src/shell/tupserverwindow.cpp \
			  ../src/shell/tservertheme.cpp \
			  ../src/server/base/logger.cpp \
			  ../src/server/core/connection.cpp \
			  ../src/server/core/server.cpp \
			  ../src/server/core/serverclient.cpp \
			  ../src/server/core/socketbase.cpp \
			  ../src/server/modules/projects/databasehandler.cpp \
			  ../src/server/modules/projects/filemanager.cpp \
			  ../src/server/modules/projects/netproject.cpp \
			  ../src/server/modules/projects/packagehandler.cpp \
			  ../src/server/modules/projects/projectmanager.cpp \
			  ../src/server/packages/items.cpp \
			  ../src/server/packages/notice.cpp \
			  ../src/server/packages/notification.cpp \
			  ../src/server/packages/package.cpp \
			  ../src/server/packages/project.cpp \
			  ../src/server/packages/projectlist.cpp \
			  main.cpp \
			  tupserverwindow_existence_test.cpp

# HEADERS for MOC (Qt meta-object compiler)
HEADERS += ../src/shell/tupserverwindow.h \
			  ../src/server/base/observer.h \
			  ../src/server/modules/communications/communicationmanager.h \
			  tupitube_server_feature_test.h

# Add all QObject-derived headers for MOC
HEADERS += \
	../src/server/core/connection.h \
	../src/server/core/server.h \
	../src/server/core/serverclient.h \
	../src/server/core/socketbase.h \
	../src/server/modules/students/studentmanager.h \
	../src/server/modules/projects/projectmanager.h \
	tupserverwindow_existence_test.h

# Ensure LIBS from main config are included
LIBS += -L/usr/local/quazip/lib -lquazip1-qt5 -L/usr/local/ffmpeg/lib -lavformat -lavcodec -lavutil -lswscale
LIBS += -L../src/server/modules/projects -lprojects
