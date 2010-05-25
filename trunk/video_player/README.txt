
                                  Introduction

This sub-project is a small video player implemented as an optional dynamically
loaded plugin for ONSlaught. It is the main part of the implementation of the
avi and mpegplay commands.
It is an optional component for two reasons:

1. The dependencies would increase the code size too much, considering how
   rarely those two commands are used.
2. The dependencies are GPL'd, which is against the spirit of the project.


                              Usage under Windows

The DLL should be placed in the same directory as the executable (".").
Depending on the used video format, different plugins may be needed. These
should be placed in ".\plugins\". It's probably not a good idea to include
all the plugins in redistributions, as their combined size exceeds 50 MiB.


                                Usage under UNIX

The build script should have taken care of everything.
