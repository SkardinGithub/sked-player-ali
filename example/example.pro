TEMPLATE = app
TARGET = skedplayer-example
QT += quick widgets
CONFIG += c++11
CONFIG += debug

# Build directory
OBJECTS_DIR = build
MOC_DIR     = build
RCC_DIR     = build
UI_DIR      = build
DESTDIR     = build

SOURCES += example.cpp
RESOURCES += qml.qrc

LIBS += -L../build -lskedplayer
LIBS += -laui

target.path = /opt/skedplayer/bin
INSTALLS += target
