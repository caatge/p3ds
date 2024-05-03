# Postal 3 dedicated server

## how to install
- get minhook from vcpkg or something because i forgot to add it to this repo (will be done later)
- add its includes and link against the lib
- you should be able to compile it now
- put the resulting executable in postal 3 root dir
- get dedicated.dll from zenoclash (https://gmod9.com/~bt/files/zeno/dedicated.dll) and put it in bin
- you should be able to start it like this: p3ds.exe -console -game p3 -maxplayers 33 +map aa

## issues
- level transitions usually don't work but sometimes they do
- random datacache.dll crashes
- like 20 million other crashes because this game was never meant to be played in multiplayer

## credits
-  IcePixelx and contributors for silver-bun
-  TsudaKageyu and contributors for Minhook
-  The Alien Swarm: Reactive Drop project for srcds_console

