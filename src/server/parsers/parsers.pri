
INCLUDEPATH += $$PARSERS_DIR

unix {
    LIBS += -L$$PARSERS_DIR -lparsers
}
