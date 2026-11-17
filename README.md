# 4chan Downloader Reborn

A maintained, portable Windows build of the original **fourchan-dl** downloader, updated for 4chan's JSON API.

**Latest tested release:** [v0.1.1](https://github.com/itsr0b0tninja/fourchan-dl-reborn/releases/latest) — portable Windows x64 ZIP.

**Current source:** `master` contains ongoing **v0.2.0 download-reliability development**. Unreleased code is not automatically installed. Check [GitHub Releases](https://github.com/itsr0b0tninja/fourchan-dl-reborn/releases) for tested downloadable versions.

## Download and updating

1. Download the portable ZIP from the latest GitHub Release.
2. Extract into its own folder and run `4chan-dl-reborn.exe`.
3. For later **published** versions use **Check for Updates** inside the program, then install the offered update.

The working v0.1.1 version has been verified by the project owner to download threads, use HTTPS, close cleanly, and update from v0.1 using its built-in updater.

## Features

- Download images and WebM attachments from 4chan threads.
- Monitor multiple threads and automatically check for new attachments.
- Resume monitoring previously opened tabs after restarting.
- Thumbnail browser, folder shortcuts, and basic per-file actions.
- Portable Qt 5 application with a built-in updater linked to this repository.

**Scope:** The Windows portable release currently includes the 4chan parser. Historical support for other imageboards in upstream code is not included in the release package.

## Project workflow

This repository intentionally uses **one source branch: `master`**.

- Work in progress is committed directly to `master` and built through GitHub Actions.
- Only verified versions are made available through GitHub **Releases**.
- Version tags (`v0.1`, `v0.1.1`, etc.) retain precise source snapshots for recovery or rollback.
- The application checks published releases, not every commit to `master`.

See [CHANGELOG.md](CHANGELOG.md) for version history.

## Credits

Fork of [fourchan-dl](https://sourceforge.net/projects/fourchan-dl/). The upstream project history and authorship remain credited.
