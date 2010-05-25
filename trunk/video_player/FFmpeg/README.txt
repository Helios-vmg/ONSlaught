
                                  Introduction

This sub-project is a small video player as an optional dynamically loaded
plugin for ONSlaught. It is the main part of the implementation of the avi and
mpegplay commands.
It is an optional component for two reasons:

1. The dependencies can only be linked properly by MinGW on Windows, and I use
   VC++ for testing, and building releases.
2. The dependencies would increase the code size too much (around 6 MiB. Four
   times the current code size), considering how rarely are those two commands
   used.


                        Supported containers and codecs

Thanks to FFmpeg, the players supports many containers and codecs without any
additional dependencies. See Formats.txt for a complete list.
Note: This list includes the non-free modules, which aren't included in the
pre-built Windows binaries.


                              Usage under Windows

The DLL should be placed in the same directory as the executable.
The pre-built binaries, including the DLL and the MinGW import libraries, don't
include the non-free modules. If these are necessary, I've made a package that
automatically builds FFmpeg and the player. You can get it from the same place
you got this package. The package should be ran on the user's computer, since
the generated bianry isn't redistributable.
A better solution might be to recode the video file/s using a format readable by
the free modules.


                                Usage under UNIX

The build script should have taken care of everything.
