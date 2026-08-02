TEMPLATE = app
TARGET   = tests

QT += core gui widgets network xml sql testlib

CONFIG += warn_on

# ──────────────────────────────────────────────────────────────────────────────
# Build flags
# ──────────────────────────────────────────────────────────────────────────────
DEFINES += TUP_DEBUG
DEFINES += TUPITUBE_TEST
DEFINES += TEST_PLUGINS_PATH=\\\"$${PLUGINS_PATH}\\\"
PLUGINS_PATH = /usr/local/tupitube.desk/lib/tupitube/plugins

include(../tupitube_config.pri)

# ──────────────────────────────────────────────────────────────────────────────
# Include paths
# ──────────────────────────────────────────────────────────────────────────────
INCLUDEPATH += \
    . \
    ../src/server/base \
    ../src/server/core \
    ../src/server/packages \
    ../src/server/parsers \
    ../src/server/modules/projects \
    ../src/server/modules/students \
    ../src/server/modules/communications \
    ../src/lib \
    /usr/local/quazip/include/quazip

# ──────────────────────────────────────────────────────────────────────────────
# Test sources — one file per domain
# ──────────────────────────────────────────────────────────────────────────────
HEADERS += \
    testhelpers.h \
    class_test.h \
    period_test.h \
    student_test.h \
    project_test.h \
    chat_test.h \
    grade_test.h \
    settings_test.h \
    tupserverwindow_existence_test.h

SOURCES += \
    main.cpp \
    class_test.cpp \
    period_test.cpp \
    student_test.cpp \
    project_test.cpp \
    chat_test.cpp \
    grade_test.cpp \
    settings_test.cpp \
    tupserverwindow_existence_test.cpp

# ──────────────────────────────────────────────────────────────────────────────
# Shell sources (needed by TupServerWindowExistenceTest)
# ──────────────────────────────────────────────────────────────────────────────
HEADERS += \
    ../src/shell/tupserverwindow.h \
    ../src/shell/firstlaunchwizard.h \
    ../src/shell/gradebookdialog.h

SOURCES += \
    ../src/shell/tupserverwindow.cpp \
    ../src/shell/tupserverwindow_gui.cpp \
    ../src/shell/tservertheme.cpp \
    ../src/shell/firstlaunchwizard.cpp \
    ../src/shell/gradebookdialog.cpp

# ──────────────────────────────────────────────────────────────────────────────
# Server core & module sources
# ──────────────────────────────────────────────────────────────────────────────
HEADERS += \
    ../src/server/packages/commandresult.h \
    ../src/server/base/observer.h \
    ../src/server/core/connection.h \
    ../src/server/core/server.h \
    ../src/server/core/serverclient.h \
    ../src/server/core/socketbase.h \
    ../src/server/modules/students/studentmanager.h \
    ../src/server/modules/projects/projectmanager.h \
    ../src/server/modules/communications/communicationmanager.h \
    ../src/server/modules/projects/filemanager.h \
    ../src/server/modules/projects/netproject.h \
    ../src/server/modules/projects/projectrenderer.h \
    ../src/lib/genericexportplugin.h

SOURCES += \
    ../src/server/base/logger.cpp \
    ../src/server/base/observer.cpp \
    ../src/server/base/packagebase.cpp \
    ../src/server/base/settings.cpp \
    ../src/server/core/connection.cpp \
    ../src/server/core/server.cpp \
    ../src/server/core/serverclient.cpp \
    ../src/server/core/socketbase.cpp \
    ../src/server/modules/communications/communicationmanager.cpp \
    ../src/server/modules/projects/databasehandler.cpp \
    ../src/server/modules/projects/filemanager.cpp \
    ../src/server/modules/projects/netproject.cpp \
    ../src/server/modules/projects/packagehandler.cpp \
    ../src/server/modules/projects/projectmanager.cpp \
    ../src/server/modules/projects/projectrenderer.cpp \
    ../src/lib/genericexportplugin.cpp \
    ../src/server/modules/students/ack.cpp \
    ../src/server/modules/students/ban.cpp \
    ../src/server/modules/students/student.cpp \
    ../src/server/modules/students/studentmanager.cpp \
    ../src/server/packages/commandresult.cpp \
    ../src/server/packages/items.cpp \
    ../src/server/packages/notice.cpp \
    ../src/server/packages/notification.cpp \
    ../src/server/packages/package.cpp \
    ../src/server/packages/project.cpp \
    ../src/server/packages/projectlist.cpp \
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

# ──────────────────────────────────────────────────────────────────────────────
# Libraries
# ──────────────────────────────────────────────────────────────────────────────
unix {
    LIBS += -L/usr/local/quazip/lib -lquazip1-qt5
    LIBS += -L/usr/local/ffmpeg/lib -lavformat -lavcodec -lavutil -lswscale
    LIBS += -L../src/server/modules/projects -lprojects
}
win32 {
    include(../quazip.win.pri)
    include(../ffmpeg.win.pri)
}

QMAKE_LFLAGS += \
    -Wl,-rpath,$$PWD/../src/lib \
    -Wl,-rpath,$$PWD/../src/server/modules/projects

