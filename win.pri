CONFIG += release
DESTDIR = release

# DEFINES += TUP_DEBUG
DEFINES += HAVE_FFMPEG

contains(DEFINES, TUP_DEBUG) {
    CONFIG += console
}
