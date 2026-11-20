# LossCat

A native Windows music player for local files. Written in C++ with Win32, Direct2D and DirectWrite.

LossCat is an early-stage project. The basics are in place, but there are still rough edges.

## What it does

- Plays FLAC, WAV, AIFF and MP3.
- Imports music into its own folder, organised by artist and album, or scans your existing music folders in place without copying them.
- Lets you browse albums, artists, songs and playlists, with album and artist pages.
- Searches your library for songs, albums and artists from the search bar.
- Keeps playlists and Favourites in `.m3u8` files, plus a **Lossless 24-bit** smart playlist that fills itself from your library.
- Shows cover art, lyrics and audio details from file tags, with the album's other songs on Now playing.
- Lets you set album covers and lyrics, and saves them into the song files' tags.
- Plays bit-perfect through WASAPI exclusive mode or ASIO, and shows whether the output is bit-perfect, resampled or converted.
- Remembers your volume, queue, position and recently played songs between sessions.
- Includes an editable queue with drag-to-reorder, removal, clear and undo, plus **Play next** and **Add to queue** actions.
- Includes shuffle, repeat and media-key controls.
- Offers a 10-band equalizer with preamp, on/off and reset controls in Settings (Shared mode, stereo only).
- Supports Unicode file paths.

## Using it

Drop files onto the window or press **Ctrl+O** to play them without importing.

**Import files** copies songs into your LossCat folder, which defaults to `Music\LossCat`:

```text
Music\LossCat\
  Artist\
    Album\
      01 Song.flac
  Playlists\
    Favourites.m3u8
```

Importing leaves the original files alone. Large imports run in the background and can be cancelled. Removing imported songs from the library sends the library copies to the Recycle Bin. Deleting a playlist only deletes the playlist, not its songs.

### Music folders

In **Settings → Library** you can add existing music folders. Their songs appear in the library where they are, without being copied. Removing one of these songs from the library hides it instead of deleting the file; hidden songs can be shown again from Settings.

You can also change the LossCat folder. Your imported songs and playlists are moved to the new location.

Scans run in the background. LossCat keeps a tag cache, so after the first scan only new or changed files are read again. Press **F5** to rescan.

### Search and queue

Use the search bar, **Ctrl+F** or **/** to find songs, albums and artists in your library. Song searches match titles, artists and albums.

Right-click a song to **Play next** or **Add to queue**. In the queue panel you can drag songs to reorder them, remove songs, clear the queue and undo queue edits.

### Equalizer

In **Settings → Equalizer**, turn on the EQ and adjust the ten bands from **31 Hz to 16 kHz**, plus the preamp. Each control ranges from **−12 to +12 dB**. Drag a slider or use the mouse wheel over it; **Reset** returns all gains and the preamp to zero. Your EQ settings are saved between sessions.

The EQ currently processes **stereo playback in Shared mode only**; it does not affect Exclusive or ASIO output. Turning it off, or leaving every control at zero, bypasses processing. Boosting bands can cause clipping; lower the preamp to leave headroom.

### Covers and lyrics are saved into your files

**Set album cover** (on an album page, or by right-clicking an album or song) takes a JPEG or PNG image, resizes it to fit within 1200 pixels, converts it to JPEG and embeds it into the tags of every song on the album, replacing any existing cover art.

On the **Lyrics** tab you can paste lyrics from the clipboard or load an `.lrc` or `.txt` file. They're saved into the song's lyrics tag. `.lrc` timestamps are kept in the tag, but lyrics are currently shown without timing.

Both change the audio files themselves, though only their tags; the audio data is untouched. This applies to every song in your library, including the original files in folders you've added. Songs that aren't in your library are never modified.

### Shortcuts

| Key | Action |
| --- | --- |
| Space | Play / pause |
| Left / Right | Seek back / forward 5 seconds |
| Ctrl+O | Open files |
| Ctrl+F or / | Focus search |
| Ctrl+L | Toggle favourite |
| Ctrl+B | Toggle sidebar |
| Ctrl+, | Settings |
| F5 | Rescan library |
| Esc | Dismiss a menu or dialog, or close the queue |

On the Lyrics tab, **Ctrl+C** copies the selected lines (or all of them), **Ctrl+A** selects everything and **Ctrl+V** saves lyrics from the clipboard.

### Where LossCat keeps things

Your settings, queue and history are in `%APPDATA%\LossCat\state.txt`, next to the tag cache (`library-cache.txt`). Deleting them resets LossCat; it doesn't touch your music.

## Building

Requires **Visual Studio 2026**, the **Desktop development with C++** workload, a Windows SDK and vcpkg integration. The projects use C++20, the v145 toolset and the `x64-windows-static` triplet.

1. Clone the repository and open `LossCat.slnx`.
2. Select **x64** and **Debug** or **Release**.
3. Build and run the **LossCat** project.

vcpkg restores TagLib from the manifest. The first build may take longer while dependencies are built.

## Current limitations

- Audio-device changes may require restarting the app.
- OGG, Opus and M4A files show up in the library but can't be played yet.
- The equalizer only works with stereo playback in Shared mode, not Exclusive or ASIO.
- Keyboard navigation and screen-reader support are incomplete.

## Code

`LossCatCore/` handles playback, metadata, the music library and playlists.
`LossCat/` contains the Windows app, input handling and rendering.

## Credits

- [miniaudio](https://github.com/mackron/miniaudio) by David Reid — audio decoding and playback. Public domain or MIT No Attribution.
- [TagLib](https://github.com/taglib/taglib) — metadata and cover art. LGPL 2.1 or MPL 1.1.
- [zlib](https://zlib.net) — compression, used by TagLib. zlib License.
- [UTF8-CPP](https://github.com/nemtrif/utfcpp) — text encoding, used by TagLib. Boost Software License 1.0.
- [ASIO SDK](https://www.steinberg.net/developers/) by Steinberg — ASIO output. GPLv3. ASIO is a trademark and software of Steinberg Media Technologies GmbH.

## License

LossCat is free software under the [GNU General Public License v3](LICENSE). Versions up to 0.2.1 were released under the MIT License.

© 2026 SegCat
