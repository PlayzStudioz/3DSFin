# 3DSFin

A homebrew Jellyfin client for the Nintendo 3DS, styled after Moonfin's clean,
card-based look. Built with devkitARM, ctrulib, and citro2d/citro3d.

## Important: scope and status

This is a **starter project**, not a finished, tested build. There is no
existing full-featured Jellyfin client for 3DS (the closest is a small
homebrew project called 3dJelly) because the hardware has real limits:

- No hardware decoder for the codecs Jellyfin normally streams. Everything
  must arrive as a low-bitrate, 3DS-friendly stream.
- No on-device subtitle renderer or arbitrary audio-track decoding.

3DSFin works around both by pushing the work to your Jellyfin server (see
"How playback works" below) instead of trying to do it on a 133MHz ARM11.

This code has **not been compiled or run on real hardware or Citra** — I
don't have a devkitARM toolchain or a 3DS in this environment. Treat it as a
correct-by-inspection scaffold you'll need to build, run, and debug yourself.
The Design section explains what's implemented vs. stubbed.

## What devkitPro actually is

devkitPro is a free, open-source toolchain: a cross-compiler (devkitARM) plus
libraries (libctru, citro2d, citro3d) that let you write C code and turn it
into a `.3dsx` (run via homebrew launcher) or `.cia` (installed like a normal
title) file. It's the standard way all 3DS homebrew gets built — think of it
like Xcode for iPhone apps, except free and command-line based.

## Setup (one-time)

1. Install devkitPro using their graphical installer:
   - Windows/macOS/Linux installers: https://github.com/devkitPro/installer/releases
   - During install, check the **3DS development** component group.
2. Add devkitPro to your PATH (the installer usually offers to do this).
   You should end up with an environment variable `DEVKITPRO` pointing at
   the install folder (e.g. `/opt/devkitpro`) and `DEVKITARM` pointing at
   `$DEVKITPRO/devkitARM`.
3. Install the extra libraries this project needs via `dkp-pacman`
   (devkitPro's package manager, installed alongside the toolchain):
   ```
   dkp-pacman -S 3ds-dev citro2d citro3d 3ds-curl 3ds-mbedtls
   ```
4. From this project folder, run:
   ```
   make
   ```
   This produces `3dsfin.3dsx` (and `3dsfin.cia` if `makerom`/`bannertool`
   are set up — optional, `.3dsx` is enough for testing via Citra or the
   Homebrew Launcher).
5. Test it in Citra (3DS emulator, easiest for iteration) or copy the
   `.3dsx` to `/3ds/` on a real, homebrew-enabled console's SD card and
   launch it from the Homebrew Launcher.

## How playback works ("server does the work")

3DSFin never decodes anything the 3DS can't handle natively:

- When you pick a movie/episode, 3DSFin fetches its `MediaSources`, which
  lists every audio track and subtitle track Jellyfin knows about.
- You pick audio + subtitle tracks **in the 3DSFin UI**, on the item details
  screen, before playback starts.
- 3DSFin then requests a transcoded stream from Jellyfin's
  `/Videos/{itemId}/master.m3u8` (or a plain progressive
  `/Videos/{itemId}/stream` fallback) with:
  - `AudioStreamIndex=<chosen>` — Jellyfin remuxes/transcodes that track down
    to stereo AAC.
  - `SubtitleStreamIndex=<chosen>` + `SubtitleMethod=Encode` — Jellyfin burns
    the subtitles directly into the video frames server-side.
  - Video capped to a resolution/bitrate the 3DS can actually decode
    (configurable in Settings; default 320x240 @ ~384kbps H.264 Baseline).
- The 3DS just plays one ordinary H.264 file. No subtitle rendering, no
  track-switching logic, no multi-audio demuxing on-device.

Switching language/subtitles mid-playback means going back to Details and
restarting playback with new track indices — there's no cheap way to hot-swap
a burned-in subtitle stream, so 3DSFin doesn't pretend to support that.

## Project layout

```
source/
  main.c            entry point, scene stack, frame loop
  app_state.h        shared session/config state
  ui_theme.h          Moonfin-style palette, spacing, fonts
  jf_http.c/.h        HTTPS client wrapper (libcurl via 3ds-curl)
  jf_json.c/.h        small hand-rolled JSON parser (no external dep)
  jf_api.c/.h         Jellyfin API calls (auth, libraries, items, streaming URLs)
  screen_login.c/.h    server address + Quick Connect / password login
  screen_home.c/.h     library list (Movies, Shows, Music, ...)
  screen_library.c/.h  poster grid for a library, paged
  screen_details.c/.h  metadata + audio/subtitle track picker + Play button
  screen_player.c/.h   video playback screen (stub — see below)
romfs/
  fonts/, gfx/         bundled UI assets (placeholders)
Makefile
```

## Implementation status

| Feature | Status |
|---|---|
| Server URL entry, connection test | Implemented |
| Password login | Implemented |
| Quick Connect login | Stubbed (API call sketch, needs polling loop) |
| Home screen / library list | Implemented (UI + API) |
| Library poster grid, paging | Implemented (UI + API) |
| Item details, cast/overview | Implemented |
| Audio + subtitle track picker | Implemented (UI), drives playback URL |
| Search | Not started |
| Playback (actual video decode) | **Stub only** — see screen_player.c. Building
  a working H.264 software decoder + display pipeline on ARM11 is the single
  biggest remaining task and deserves its own focused pass rather than a
  guessed implementation here. |
| Resume points / "continue watching" | Not started |
| Music playback | Not started |
| Live TV | Deliberately out of scope (server rarely has a tuner reachable
  from a 3DS-friendly transcode path; revisit later if wanted) |

## Suggested build order from here

1. Get `make` producing a `.3dsx` that boots to the login screen in Citra.
2. Wire up real Jellyfin auth against a test server and confirm the home/library
   screens populate.
3. Confirm the details screen's track picker builds a correct playback URL
   (print/log it — you can literally paste it into VLC on a PC to sanity-check
   before touching on-device video).
4. Tackle `screen_player.c` video decode as its own milestone.
