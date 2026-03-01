
INCLUDEPATH += $$SERVERMODULES_DIR

LIBS += -L$$SERVERMODULES_DIR/users -lusers -L$$SERVERMODULES_DIR/projects -lprojects -L$$SERVERMODULES_DIR/communications -lcommunications
