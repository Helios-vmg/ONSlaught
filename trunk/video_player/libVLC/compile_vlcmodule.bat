@echo off
if not exist bin-win32 (
	md bin-win32
)
g++ -shared archive_access.cpp -Iinclude/vlc/plugins -Llib-win32 -lvlccore -O3 -s -DDIR=void -D__PLUGIN__ -o bin-win32/libaccess_nons_plugin.dll
