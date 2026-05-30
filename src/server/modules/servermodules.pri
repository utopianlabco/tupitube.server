
INCLUDEPATH += $$SERVERMODULES_DIR

unix {
    LIBS += -L$$SERVERMODULES_DIR/students -lstudents -L$$SERVERMODULES_DIR/projects -lprojects -L$$SERVERMODULES_DIR/communications -lcommunications
}
