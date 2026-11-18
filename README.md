# 4Chan Downloader Reborn

**4Chan Downloader Reborn** is a portable Windows app for downloading files from 4chan threads and keeping track of new posts. It's an updated version of the original [fourchan-dl](https://sourceforge.net/projects/fourchan-dl/), with support for 4chan's JSON API.

[**Download the latest release**](https://github.com/itsr0b0tninja/fourchan-dl-reborn/releases/latest) · [Changelog](CHANGELOG.md)

## Features

- **Thread monitoring:** Open several threads in separate tabs, automatically rescan for new attachments, or refresh a thread manually.
- **Images and video:** Download images, WebM and MP4 files. Choose whether to download videos globally or override the setting for individual threads.
- **Reliable downloads:** Configurable concurrent downloads, automatic retries and retry delays, including handling for temporary rate limits.
- **Thumbnail browser:** Browse saved images as thumbnails, rebuild the thumbnail cache, and open or remove individual files.
- **Image viewer:** View images inside the app, navigate between them, rotate them, or open them in an external viewer.
- **Folder management:** Choose a download location per thread, save folder shortcuts, retain original filenames, and move a thread's files to the configured destination.
- **Thread Overview:** See thread status and image counts in one place, filter threads by status or board (such as `/b/` and `/r9k/`), and mark threads as viewed.
- **Session and history:** Reopen tabs from your previous session and restore recently closed threads from the History menu.
- **Controls and preferences:** Start, pause or stop downloads; configure rescan intervals, a file blacklist, thumbnail settings and network proxy options.
- **Desktop integration:** Light and dark themes, system tray support, and built-in update checks.

## Getting started

1. Download the **Windows x64 portable ZIP** from the [latest release](https://github.com/itsr0b0tninja/fourchan-dl-reborn/releases/latest).
2. Extract it into its own folder and run `4chan-dl-reborn.exe`. No installer is needed.
3. Select **Add thread**, paste a 4chan thread URL, choose where to save the files, and start downloading.

The app supports 4chan and its official 4channel.org addresses. Other imageboards are not supported.

To update an existing installation, use **Check for Updates** in the app. For a fresh installation, extract the ZIP to a new folder rather than over an existing copy containing your settings or downloads.

## Licence and credits

Released under the [GNU General Public License v3](LICENSE). Third-party components retain their own licences.

Based on the original [fourchan-dl](https://sourceforge.net/projects/fourchan-dl/). Credit for the original code remains with its authors, alongside the contributors to this version.
