#!/bin/bash

STAGING_DIR=$(readlink -e ${STAGING_DIR:-"${PWD}/../../host/usr/mipsel-buildroot-linux-gnu/sysroot/"})
TARGET_DIR=$(readlink -e ${TARGET_DIR:-"${PWD}/../../target/"})
QMAKE=$(readlink -e ${QMAKE:-"${PWD}/../../host/usr/bin/qmake"})

CONFIG=""
CONFIG="${CONFIG} example"
#CONFIG="${CONFIG} server"
CONFIG="${CONFIG} debug"

usage() {
  echo "Usage: $(basename $0) <clean|qmake|make|install>"
  echo
  exit
}

[[ $# < 1 ]] && usage
case $1 in
  clean)
    for i in $(find . -name Makefile); do
      rm -f $i;
    done;
    [ -d build ] && rm -rf build;
    [ -d example/build ] && rm -rf example/build;
    [ -f example/skedplayer_plugin_import.cpp ] && rm example/skedplayer_plugin_import.cpp;
    [ -f example/skedplayer_qml_plugin_import.cpp ] && rm example/skedplayer_qml_plugin_import.cpp;
    [ -d server/build ] && rm -rf server/build;
    ;;
  qmake)
    ${QMAKE} "CONFIG+=${CONFIG}"
    ;;
  make)
    make
    ;;
  install)
    make install INSTALL_ROOT=${STAGING_DIR};
    make install INSTALL_ROOT=${TARGET_DIR};
    ;;
  *)
    usage
    ;;
esac
