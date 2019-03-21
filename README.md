# SKED Media Player for ALI platform

## Compiling in ALI BuildRoot

```shell
cd /path/to/ali/buildroot/output/build/
git clone <repo_path>
cd sked-player-ali
./br_ali.sh clean
./br_ali.sh qmake
./br_ali.sh make
./br_ali.sh install
```

## Install a dependent lib

In ALI buildroot root path:

```shell
cp -d output/staging/usr/lib/libatomic.* output/target/usr/lib/
```

## Running example in ALI board

Mount USB disk (which store media files) to path start with "/media/".

```shell
/opt/sked/player/bin/skedplayer-example -platform directfb -plugin evdevkeyboard
```
