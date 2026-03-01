PRI_FILE = ../../tupitube_config.pri
exists ($$PRI_FILE) {
    include ($$PRI_FILE)
} else {
    error("[shell] Please, run configure first")
}

QT += network xml sql

INCLUDEPATH += ../server/base ../server/core ../server/packages ../server/parsers ../server/modules/users ../server/modules/projects ../server/modules/backups ../server/modules/ban

LIB_DIR = ../lib
include($$LIB_DIR/lib.pri)
