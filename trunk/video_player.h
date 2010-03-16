/*
* Copyright (c) 2009, Helios (helios.vmg@gmail.com)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice,
*       this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * The name of the author may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY HELIOS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
* EVENT SHALL HELIOS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* C-friendly header follows. */

#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H
#include <SDL/SDL.h>

#define NONS_SYS_WINDOWS (defined _WIN32 || defined _WIN64)
#define NONS_SYS_LINUX (defined linux || defined __linux)
#define NONS_SYS_UNIX (defined __unix__ || defined __unix)

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#if NONS_SYS_WINDOWS
#ifndef _USRDLL
#define DLLexport __declspec(dllexport)
#else
#define DLLexport __declspec(dllimport)
#endif
#else
#define DLLexport
#endif

typedef SDL_Surface *(*playback_cb)(volatile SDL_Surface *screen,void *user_data);
#define PLAYBACK_FUNCTION_PARAMETERS \
	SDL_Surface *screen,\
	const char *input,\
	int *stop,\
	void *user_data,\
	int *toggle_fullscreen,\
	int *take_screenshot,\
	playback_cb fullscreen_callback,\
	playback_cb screenshot_callback,\
	int print_debug
#define PLAYBACK_FUNCTION_NAME C_play_video
#define PLAYBACK_FUNCTION_NAME_STRING "C_play_video"
#define PLAYBACK_FUNCTION_SIGNATURE \
	EXTERN_C DLLexport int PLAYBACK_FUNCTION_NAME(PLAYBACK_FUNCTION_PARAMETERS)

/*
 * video_playback_fp:
 *
 * Description (C_play_video()):
 *     Plays a video using libavcodec to parse and decode the file, SDL for
 *     video output, and OpenAL for audio output.
 *     The video file must contain at least one video stream. If it contains
 *     more than one, the function chooses the first one it finds.
 *     An audio stream is optional.
 *     Subtitles are not rendered.
 *     The function requires exclusive access to the destination surface, so if
 *     there are other threads running in the caller program that could read or
 *     write to the surface, either they should be stopped, or accesses to the
 *     surface should be locked with a mutex.
 *     See video_player/formats.txt for a list of formats and codecs supported
 *     by the libav* libraries.
 *     If the video and the destination surface are of different sizes, a
 *     bilinear interpolation (provided by libswscale) is applied to each frame.
 *     If the video and the destination surface are of different width-to-height
 *     ratios, the video is sized and positioned so that it fits completely
 *     inside the surface. For example, playing a 1920x1080 (16:9) video on an
 *     1024x768 (4:3) surface would produce a letterboxed picture, while doing
 *     the opposite would produce a pillarboxed picture.
 *     The function does not clear (i.e. fill with black) the destination
 *     surface before or after. That job is left to the caller.
 *     Like any video-related code, the function is rather resource-consuming.
 *     Depending on the screen size of the video, the function can use anywhere
 *     between ~20 MiB (512x382) and 60 MiB (720p high definition video).
 *
 * Parameters:
 *     SDL_Surface *screen
 *         A pointer to the SDL_Surface that the video will be rendered to. The
 *         surface *must* be the real screen (i.e. the pointer returned by
 *         SDL_SetVideoMode()).
 *     const char *input
 *         The path to the video file to be played.
 *     int *stop
 *         The function continually checks the int pointed to by stop and
 *         returns as soon as possible when its value becomes !0. The pointer is
 *         not checked, so passing an invalid pointer (including NULL) is likely
 *         to cause a segmentation fault.
 *     void *user_data
 *         This pointer will be passed to the callback functions if they are
 *         called.
 *     int *toggle_fullscreen
 *         Serves the same purpose as 'stop'. The pointer is also assumed to be
 *         valid.
 *         When the int pointed becomes !0, the function pointed to by
 *         'fullscreen_callback' is called. The int is reset back to zero
 *         immediately.
 *     int *take_screenshot
 *         Same as 'toggle_fullscreen', but calls screenshot_callback.
 *     playback_cb fullscreen_callback
 *     playback_cb screenshot_callback
 *         See above. The only other noteworthy thing to say is that these
 *         pointers don't need to be valid. If NULL is passed, the pointer will
 *         not be dereferenced even if the associated variable becomes !0.
 *         The functions will be called from threads other than the caller's.
 *         This should be considered when writing the callback to avoid race
 *         conditions and deadlocks.
 *         A callback is supposed to return an updated pointer to the screen.
 *         If the callback doesn't relocate the screen (e.g. by not calling
 *         SDL_SetVideoMode()), it must return the value of 'screen', possibly
 *         explicitly casted to (SDL_Surface *).
 *
 *     Note: Despite their names, The functions can be used for purposes other
 *         than coming back and forth from fullscreen mode or taking a
 *         screenshot. The parameters were so called merely because that's what
 *         the parent project uses them for.
 *         Using these parameters may place the caller program under the GPL.
 *
 * Returns:
 *     An error code describing the execution result. See the #defines below.
 *     Note that two or more of these codes may be bitwise-OR'd.
 *
 */
typedef int (*video_playback_fp)(PLAYBACK_FUNCTION_PARAMETERS);
PLAYBACK_FUNCTION_SIGNATURE;

/* Return codes for C_play_video() */
#define PLAYBACK_NO_ERROR					0x0000
#define PLAYBACK_FILE_NOT_FOUND				0x0001
#define PLAYBACK_STREAM_INFO_NOT_FOUND		0x0002
#define PLAYBACK_NO_VIDEO_STREAM			0x0004
#define PLAYBACK_NO_AUDIO_STREAM			0x0008 /* unused */
#define PLAYBACK_UNSUPPORTED_VIDEO_CODEC	0x0010
#define PLAYBACK_UNSUPPORTED_AUDIO_CODEC	0x0020
#define PLAYBACK_OPEN_VIDEO_CODEC_FAILED	0x0040
#define PLAYBACK_OPEN_AUDIO_CODEC_FAILED	0x0080
#define PLAYBACK_OPEN_AUDIO_OUTPUT_FAILED	0x0100
#endif
