# LossCat

A native Windows music player for local files. Written in C++ with Win32, Direct2D and DirectWrite.

LossCat is an early-stage project. The basics are in place, but there are still rough edges.

## What it does

- Plays FLAC, WAV, AIFF and MP3.
- Organises imported music by artist and album.
- Lets you browse albums, artists, songs and playlists.
- Keeps playlists and Favourites in `.m3u8` files.
- Shows cover art, embedded lyrics and audio information from file tags.
- Displays the source and output sample rates, including whether resampling is happening.
- Includes a queue, shuffle, repeat and media-key controls.

## Using it

Drop files onto the window or press **Ctrl+O** to play them without importing.

**Import files** copies songs into your Windows Music folder under `LossCat`:

```text
Music\LossCat\
  Artist\
    Album\
      01 Song.flac
  Playlists\
    Favourites.m3u8
```

Importing leaves the original files alone. Removing imported songs from the library sends the library copies to the Recycle Bin. Deleting a playlist only deletes the playlist, not its songs.

### Shortcuts

| Key | Action |
| --- | --- |
| Space | Play / pause |
| Left / Right | Seek back / forward 5 seconds |
| Ctrl+O | Open files |
| Ctrl+L | Toggle favourite |
| Ctrl+B | Toggle sidebar |
| Ctrl+, | Settings |
| F5 | Rescan library |
| Esc | Dismiss a menu or dialog, or close the queue |

## Building

Requires **Visual Studio 2026**, the **Desktop development with C++** workload, a Windows SDK and vcpkg integration. The projects use C++20, the v145 toolset and the `x64-windows-static` triplet.

1. Clone the repository and open `LossCat.slnx`.
2. Select **x64** and **Debug** or **Release**.
3. Build and run the **LossCat** project.

vcpkg restores TagLib from the manifest. The first build may take longer while dependencies are built.

## Current limitations

- Large imports can block the interface.
- Some non-Latin file paths may fail to open or import.
- Volume, queue and recently played history are not restored between sessions.
- Playback uses shared-mode audio; there is no exclusive mode.
- Audio-device changes may require restarting the app.
- OGG, Opus and M4A playback is not supported yet.
- Keyboard navigation and screen-reader support are incomplete.

## Code

`LossCatCore/` handles playback, metadata, the music library and playlists.
`LossCat/` contains the Windows app, input handling and rendering.

## Credits

- [miniaudio](https://github.com/mackron/miniaudio) by David Reid — audio decoding and playback. Public domain or MIT No Attribution.
- [TagLib](https://github.com/taglib/taglib) — metadata and cover art. LGPL 2.1 or MPL 1.1.

© 2026 SegCat
