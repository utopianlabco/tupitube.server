
INCLUDEPATH += $$SERVER_DIR

unix {
    LIBS += -L$$SERVER_DIR/core -lserver -L$$SERVER_DIR/base -lbase \
            -L$$SERVER_DIR/packages -lserverpackage -L$$SERVER_DIR/parsers -lparsers
}
win32 {
    LIBS += -Wl,--start-group \
        -L$$SERVER_DIR/core/release -lserver \
        -L$$SERVER_DIR/modules/students/release -lstudents \
        -L$$SERVER_DIR/modules/projects/release -lprojects \
        -L$$SERVER_DIR/modules/communications/release -lcommunications \
        -L$$SERVER_DIR/base/release -lbase \
        -L$$SERVER_DIR/packages/release -lserverpackage \
        -L$$SERVER_DIR/parsers/release -lparsers \
        -L$$SERVER_DIR/../lib/release -ltuputils \
        -Wl,--end-group
}

SERVERMODULES_DIR = $$SERVER_DIR/modules
include($$SERVERMODULES_DIR/servermodules.pri)

#LIBS += -L$$SERVER_DIR/base -lbase -L$$SERVER_DIR/packages -lserverpackages -L$$SERVER_DIR/parsers -lparsers
