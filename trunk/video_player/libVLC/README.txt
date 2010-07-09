See ../FFmpeg/README.txt for an introduction.

This is a video player in experimental stage based on libVLC. The code itself
is stable, probably more stable than the FFmpeg-based player, but it's very
hard to get working under UNIX (honestly, I haven't been able to do it), and I
won't bore you with details, but it requires doing some unsavory things to get
working at all anywhere due to limitations in the libVLC interface. It's also
slightly more resource-hungry than the other player.
It does have the advantage of supporting soft subtitles our of the box, and it
works flawlessly under WINE (paradoxically, it works even better than
VLC Player itself).


                        How to get working under Windows

The following steps will involve compiling the plugin from source. It's
recommended to have both a version of VC++ and MinGW installed, but you'll
definitely need MinGW.
1. Go to http://www.videolan.org/vlc/ and get any of the installer-less
   packages.
2. X will be the directory where the engine is, Y where the ONSlaught-*-src
   package is, and Z where the VLC package was extracted.
   After extracting, copy:
   A. Z\libvlc.dll and Z\libvlccore.dll to X\.
   B. Z\plugins\  to X\.
   C. Z\sdk\include\ to Y\video_player\libVLC\.
   D. Z\sdk\lib\ to Y\video_player\libVLC\.
   E. Y\lib-win32\SDL.* to Y\video_player\libVLC\lib-win32\
3. Rename Y\video_player\libVLC\lib to lib-win32.
4. Rename the files inside Y\video_player\libVLC\lib-win32
     libvlccore.dll.a -> libvlccore.lib
     libvlc.dll.a     -> libvlc.lib
   You may delete the rest of the files in lib-win32 if you want.
5. Using the console, navigate to Y\video_player\libVLC\
6. Make sure g++ (the MinGW C++ compiler) is available from the command line.
   Run
   
   g++ --version
   
   If you get an error, run
   
   set MINGW=<the MinGW installation directory>
   set PATH=%PATH%;%MINGW%\bin
   
   Now it should be available.
7. Run

   compile_vlcmodule.bat

   Copy Y\video_player\libVLC\bin-win32\libaccess_nons_plugin.dll which should
   have been generated to X\plugins\.
8. Compile the plugin itself using the Visual C++ project. If you want to
   compile with MinGW, Code::Blocks can convert Visual C++ projects to its own
   format.
   It's recommended to use the Release configuration, but the Debug
   configuration should also work.
9. Copy Y\video_player\libVLC\bin-win32\video_player.dll to X\.