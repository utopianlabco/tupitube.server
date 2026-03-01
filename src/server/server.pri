
INCLUDEPATH += $$SERVER_DIR 

LIBS += -L$$SERVER_DIR/core -lserver -L$$SERVER_DIR/base -lbase -L$$SERVER_DIR/packages -lserverpackage -L$$SERVER_DIR/parsers -lparsers

SERVERMODULES_DIR = $$SERVER_DIR/modules
include($$SERVERMODULES_DIR/servermodules.pri)

#LIBS += -L$$SERVER_DIR/base -lbase -L$$SERVER_DIR/packages -lserverpackages -L$$SERVER_DIR/parsers -lparsers
