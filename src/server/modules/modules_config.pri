PRI_FILE = ../../../tupitube_config.pri
exists ($$PRI_FILE) {
    include ($$PRI_FILE)
} else {
    error("[modules] Please, run configure first")
}

INCLUDEPATH += ../ ../../base ../../core ../../parsers ../ban ../users ../../packages ../../../lib ../backups
