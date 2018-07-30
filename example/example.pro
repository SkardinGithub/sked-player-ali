TEMPLATE = app
TARGET = skedplayer
QTPLUGIN += qdirectfb qevdevkeyboardplugin
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
LIBS += -L$$[QT_SYSROOT]/usr/qml/QtQuick.2 -lqtquick2plugin
LIBS += -L$$[QT_SYSROOT]/usr/qml/Qt/labs/folderlistmodel -lqmlfolderlistmodelplugin
LIBS += -laui

target.path = /opt/$${TARGET}/bin
INSTALLS += target
