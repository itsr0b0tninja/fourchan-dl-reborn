# 4Chan Downloader Reborn

A maintained, portable Windows build of the original **fourchan-dl** downloader, updated for 4chan's JSON API.

**Latest published release:** [Download the Windows x64 portable version](https://github.com/itsr0b0tninja/fourchan-dl-reborn/releases/latest).

**Current source:** `master` always shows the newest source. Releases are automatically published after successful Windows compilation, SSL verification, and the streaming file test when the version number increases.

## Download and updating

1. Download the portable ZIP from the latest GitHub Release.
2. Extract into its own folder and run `4chan-dl-reborn.exe`.
3. For later **published** versions use **Check for Updates** inside the program, then install the offered update.

For milestone releases such as v0.5.0, the GitHub Release also includes a complete portable ZIP. Existing installations can continue using Check for Updates. The portable ZIP should be extracted to a fresh directory, not over an installation containing personal data.

## Features

- Download images and WebM attachments from 4chan threads.
- Monitor multiple threads and automatically check for new attachments.
- Resume monitoring previously opened tabs after restarting.
- Thumbnail browser, folder shortcuts, and basic per-file actions.
- Portable Qt 5 application with a built-in updater linked to this repository.
- Robust streamed attachment downloads, retries, and video CDN request headers.
- Qt5 Fusion Light/Dark themes and the simplified Thread Overview status filter.
- v0.5.0: curated portable image plugins and narrowly scoped removal of known obsolete Qt4/old updater files; user data is never targeted.

**Scope:** The Windows portable release currently includes the 4chan parser. Historical support for other imageboards in upstream code is not included in the release package.

The current source tree contains only the supported Windows GUI, 4chan parser, standalone updater and active build/tests. Unshipped historical parsers and the discontinued console frontend were removed from `master` during repository cleanup; previous versions remain available through Git history.

## Project workflow

This repository intentionally uses **one source branch: `master`**.

- Work in progress is committed directly to `master` and built through GitHub Actions.
- New versions are automatically published through GitHub **Releases** once CI succeeds; local application testing follows publication.
- Version tags (`v0.1`, `v0.1.1`, etc.) retain precise source snapshots for recovery or rollback.
- The application checks published releases, not every commit to `master`.

See [CHANGELOG.md](CHANGELOG.md) for version history.

## Licensing

The project remains available under GNU GPL version 3. The [LICENSE](LICENSE) file contains the full, unaltered GPLv3 text followed by an updated project-specific licensing and attribution notice. The original authors and subsequent contributors retain their respective copyrights. Bundled third-party components retain their own licences.

The main window displays **4Chan Downloader Reborn** without a version suffix. The installed version is still available in Program Information and remains essential to update checking and releases.

## Credits

Fork of [fourchan-dl](https://sourceforge.net/projects/fourchan-dl/). The upstream project history and authorship remain credited.
