
INCLUDEPATH += $$PACKAGES_DIR

unix {
    LIBS += -L$$PACKAGES_DIR -lserverpackage
}
