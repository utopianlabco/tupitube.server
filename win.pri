CONFIG += release

# Place the binary in bin/ at the project root, mirroring the Linux layout.
# TARGET is the clean Windows executable name; the .exe extension is added
# automatically by qmake/mingw32-make.
DESTDIR = ../../bin
TARGET  = tupitube.server

# DEFINES += TUP_DEBUG
DEFINES += HAVE_FFMPEG

contains(DEFINES, TUP_DEBUG) {
    CONFIG += console
}
