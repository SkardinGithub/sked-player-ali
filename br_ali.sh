#!/bin/bash

export STAGING_DIR=$(readlink -e ${STAGING_DIR:-"${PWD}/../../host/usr/mipsel-buildroot-linux-gnu/sysroot/"})
export TARGET_DIR=$(readlink -e ${TARGET_DIR:-"${PWD}/../../target/"})

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
    ;;
  qmake)
    ../../host/usr/bin/qmake
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
