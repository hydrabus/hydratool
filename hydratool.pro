CONFIG(debug, debug|release) {
QMAKE_CXXFLAGS_DEBUG += -g3 -O0
DEFINES += QT_NO_DEBUG
DEFINES += QT_NO_DEBUG_OUTPUT
message("DEBUG! QT_NO_DEBUG_OUTPUT")
#message("DEBUG! QT_DEBUG_OUTPUT")
} else {
# VisualStudio optimization /Ot favor speed /Ox max optimization/Oy omit frame pointer x86
#QMAKE_CXXFLAGS += /Ot /Ox /Oy
# VisualStudio optimization /Os favor size
#QMAKE_CXXFLAGS += /Os
DEFINES += QT_NO_DEBUG
DEFINES += QT_NO_DEBUG_OUTPUT
message("RELEASE!")
}

QT += core gui
QT += widgets
#QT += network
QT += serialport

TARGET = hydratool
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp \
    hydratooldialog.cpp \
    history_line_edit.cpp \
    directdiskdialog.cpp \
    sniff/sniff.c \
    sniff/sniff_ascii.c \
    terminaldialog.cpp

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    version.h \
    history_line_edit.hpp \
    directdiskdialog.h \
    sniff/sniff.h \
    sniff/sniff_ascii.h \
    hydratooldialog.h \
    terminaldialog.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui \
    directdiskdialog.ui \
    hydratooldialog.ui \
    terminaldialog.ui

RESOURCES += \
    hydratool.qrc

RC_FILE = resources.rc
