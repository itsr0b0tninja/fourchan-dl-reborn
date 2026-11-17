# 4chan Downloader Reborn

A maintained, portable Windows build of the original **fourchan-dl** downloader, updated for 4chan's JSON API.

**Latest published release:** [Download the Windows x64 portable version](https://github.com/itsr0b0tninja/fourchan-dl-reborn/releases/latest).

**Current source:** `master` always shows the newest source. Releases are automatically published after successful Windows compilation, SSL verification, and the streaming file test when the version number increases.

## Download and updating

1. Download the portable ZIP from the latest GitHub Release.
2. Extract into its own folder and run `4chan-dl-reborn.exe`.
3. For later **published** versions use **Check for Updates** inside the program, then install the offered update.

The owner confirmed successful in-program updating and verified the v0.2.0 attachment-failure UI on Windows. v0.2.1 adds streaming file writes, faster rescan lookups, and more efficient blacklist checks without redesigning the interface.

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
- New versions are automatically published through GitHub **Releases** once CI succeeds; local application testing follows publication.
- Version tags (`v0.1`, `v0.1.1`, etc.) retain precise source snapshots for recovery or rollback.
- The application checks published releases, not every commit to `master`.

See [CHANGELOG.md](CHANGELOG.md) for version history.

## Credits

Fork of [fourchan-dl](https://sourceforge.net/projects/fourchan-dl/). The upstream project history and authorship remain credited.
