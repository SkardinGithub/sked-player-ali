TEMPLATE = app
TARGET = skedplayer-example
QT += quick widgets
CONFIG += c++11
CONFIG += qtquickcompiler
CONFIG += debug

# Build directory
OBJECTS_DIR = build
MOC_DIR     = build
RCC_DIR     = build
UI_DIR      = build
DESTDIR     = build

SOURCES += example.cpp
RESOURCES += qml.qrc

PRE_TARGETDEPS += ../build/libskedplayer.a
LIBS += ../build/libskedplayer.a
LIBS += -laui
static {
  QTPLUGIN += qdirectfb qevdevkeyboardplugin
  LIBS += -L$$[QT_SYSROOT]/usr/qml/QtQuick.2 -lqtquick2plugin
  LIBS += -L$$[QT_SYSROOT]/usr/qml/Qt/labs/folderlistmodel -lqmlfolderlistmodelplugin
  DEFINES += QT_STATIC
}

target.path = /opt/sked/player/bin
INSTALLS += target
