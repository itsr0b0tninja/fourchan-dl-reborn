# Changelog — 4chan Downloader Reborn

## v0.4.5 — Status header menu hotfix
- Fix the missing Status filter popup: Qt disables clickable QTreeView headers when sorting is disabled, so re-enable header clicks after setting sorting off.
- Keep all four overview columns manually resizable and sorting disabled. Preserve existing filter preferences and categories.
- Add a Windows Qt regression test that physically clicks the Status header and verifies that sectionClicked(2) fires without enabling sorting or changing column widths.
- This release only corrects the v0.4.4 header interaction; no additional overview features or UI redesign.

## v0.4.4 — Manual overview columns and Status header filter
- Make Name, Images, Status and URI columns independently draggable, without ResizeToContents or automatic final-column stretching. Text may be clipped; full values are available via cell tooltips.
- Clicking the existing Status column heading opens a single-choice status filter menu with counts for all open threads. Every status category appears even when its count is zero; All statuses clears filtering.
- Show the active filter in the Status heading and a check beside the selected menu entry. Classify changing retry times as Retry Pending and removed/404 threads as Ended / Removed.
- Update row visibility automatically as download statuses change. Filtering never stops or removes a thread, and hides rows instead of reordering them.
- Bind each overview row to its thread widget rather than assuming its visible position matches the tab index. Preserve the active-thread marker and new-image highlighting.
- Remember the filter and all four manually chosen column widths across launches. Debounce width persistence during drags and do not reset widths when the main Settings Apply button is used.
- Add an offscreen Qt regression test for status classification (including zero-result filters), manual headers and stable filtered-row indexes.
- Preserve the existing docked overview and thread settings layout; no search, sorting, extra toolbar or additional thread-management actions.

## v0.4.2 — Consistent Fusion themes and Settings Apply
- Use Qt Fusion controls for both Light and Dark, retaining Dark's charcoal palette and selection colours while giving Light a soft grey/white palette and matching control styling.
- Preserve Light as the default theme, and remember the selected theme between launches.
- Stop bundling qwindowsvistastyle.dll and the optional styles directory. Existing installations can delete an old styles directory manually; do not delete user files during updates.
- Add an Apply button immediately to the left of OK, retaining the existing OK/Cancel buttons and Settings window layout.
- Apply saves settings and refreshes the running application without closing Settings; OK saves and closes; Cancel discards unapplied edits.
- Enable Apply only when an editable option differs from its last successfully applied value, and disable it again after saving or reverting.
- Remember the last open Settings tab across restarts without marking other settings dirty.
- Keep the separate timeout-value and folder-shortcut editors' existing independent save behaviour.
- Preserve existing settings.ini keys, thread downloads, the thread mover, automatic updates, and the internal viewer.
- Extend Windows Qt theme smoke tests for both Fusion palettes, live switching and layout/colour restoration.

## v0.4.1 — Optional charcoal dark mode
- Add a two-option Theme preference under Settings → General: Light (default) and Dark.
- Preserve the original Windows Light look and default for existing installations; do not follow the Windows system theme automatically.
- Introduce a consistent charcoal Qt Fusion palette with blue highlights, accessible selection contrast and dark menus, tabs, progress bars, scrollbars, tooltips, and thread/thumbnail surfaces.
- Apply theme changes immediately when Settings are saved and persist the preference across restarts.
- Make thread overview normal/new-image text and the logging warning theme-aware; preserve the existing Light colours.
- Apply globally, including dialogs and the internal image viewer; no changes to the downloader, thread mover, viewer functionality or thumbnail generation.
- Add a Qt Widgets smoke test covering Light defaults, Dark palette, theme-aware status colours, live switching, native-Light restoration and invalid preference fallback.

## v0.3.2 — One-click thread move and close
- Replace only the Open Image Preview toolbar button with a folder/arrow Move Thread button. Preserve the internal image viewer, its configuration, thumbnail double-click and Open File actions.
- Add a single 'Move threads to' destination root in Settings → Thread.
- Preserve the parser DLL's original folder structure (such as %b/%n/) when moving to the new root.
- Stop only the selected thread's downloads, move only its recorded, present attachments, then close its tab without adding it to History.
- Run moves in a background worker to keep the main UI responsive. Do not require confirmation or a destination picker per thread.
- Preflight conflicts, preserve unrelated files, accept already-identical destinations, verify transfers using SHA-256 and avoid overwriting different files.
- Copy and verify all files before deleting originals; on a failed copy, clean up new copies and keep the thread open with a visible error. Failed source cleanup also leaves the tab open and retryable.
- Disable the button when no destination root is configured; never move thumbnails or alter downloaded media settings.
- Add a Windows smoke test for DLL path mapping, verified copying, file conflicts and copy-stage rollback.

## v0.3.1 — Configurable, session-based automatic update checks
- Add three default-on preferences under Settings → General: check GitHub on startup, check every X days (default: one day), and notify of new releases.
- Start a fresh periodic countdown on every application launch and after each scheduled check; do not persist the countdown across sessions.
- Keep scheduled checks lightweight and prevent overlapping GitHub checks; scheduled failures are logged without intrusive notification.
- Show in-app notification once per newly detected version, remembered across application restarts; retain existing Update Available action and detail dialog.
- Preserve the manual Check for Updates action regardless of automatic preferences.
- Never automatically download executable updates or start installation.

## v0.2.6 — Global/per-thread video settings, attachment counts and corrected thread status
- Add a Download Manager preference to download MP4/WebM videos by default (enabled).
- Add per-thread video override next to the Rescan Thread controls: global / download / skip, saved with thread session settings.
- Add a compact, always-visible attachment breakdown above thumbnails: downloaded images, downloaded videos, skipped video count and total downloaded/discovered.
- Skipped videos remain in thread totals but do not block Finished or count as downloaded, missing or failed.
- Apply changes immediately: cancel pending video transfers when disabled, preserve completed videos, and queue previously skipped files when re-enabled.
- Keep ordinary image downloads and thread rescans running during video rate-limit cooldowns.
- Thread Overview statuses: Downloading for runnable work; Rendering for pending thumbnails; Finished for completed monitored threads; Finished (retry HH:mm:ss) only when all outstanding attachments are awaiting a retry. Preserve 404/stopped statuses.
- Do not label periodic thread monitoring as a retry and do not display a separate cooldown label in the main status bar.
- Retain thumbnail completion fixes and publish as a small incremental executable update.

## v0.2.5 — Thread Overview retry statuses and rendering completion
- Show per-thread attachment retries directly in the Thread Overview Status column: Downloading (scheduled retry HH:mm:ss).
- When all media is downloaded, display Finished (scheduled retry HH:mm:ss) if monitoring is due to rescan.
- Correct duplicate thumbnail queue paths which left some threads displaying Rendering after thumbnails were complete.
- Immediately clear finished thumbnails from the pending list; restore Finished after an unchanged rescan.
- Preserve existing Stopped, Blocked, missing and 404 thread states.
- Remove the unwanted global status-bar retry indicator from v0.2.4.
- Incremental executable-only release; retain parser v0.2.2.

## v0.2.4 — Persistent video retry time
- Show the exact local retry deadline in a permanent status-bar indicator while 4chan's video cooldown is active.
- Restore that indicator on restart from the persisted cooldown deadline and hide it when the cooldown ends.
- Log the absolute date and time of the next retry, as well as the relative delay.
- Reuse the v0.2.2 parser and publish only the updated main executable and release metadata.

## v0.2.3 — HTTP 429 retry handling and incremental update
- Honour Retry-After (seconds or HTTP-date), with a short margin and 24-hour cap.
- HTTP 429 pauses downloads without consuming the ordinary retry budget.
- Persist a shared video cooldown; allow images and thread updates to continue.
- Display the expected video retry time locally and test the policy in Windows CI.
- Publish the changed executable and manifest individually, keep the v0.2.2 parser as a reusable component, and distribute LICENSE.txt separately. Only x.y.0 releases require portable ZIPs.

## v0.2.2 — Released (video CDN response pending Windows user verification)
- Address a reproducible pattern where images download but MP4/WebM return HTTP 429 in the desktop application even when the same URLs open in a browser.
- Use a browser-compatible User-Agent in place of the legacy Wget/1.12 default, honouring intentional custom user agents.
- Send video CDN requests with a board Referer and a complete-byte-range request (bytes=0-); preserve original-size validation and atomic temporary-file downloads.
- Add diagnostic logging for video HTTP 429 replies and a Windows request-header regression test.
- No changes to global download delays, threading limits, or user interface layout.

## v0.2.1 — Release
- Write attachments progressively to atomic temporary files rather than retaining entire media bodies in RAM; verify expected byte counts before committing.
- Index known attachment URLs and original filenames and avoid repeated full-list scans while queuing attachments and rescanning threads.
- Speed up case-insensitive blacklist lookups and pace periodic HEAD cleanup checks, avoiding a large startup burst.
- Defer download queue processing until request handlers have registered IDs; batch bursts of restored requests.
- Add a Windows CI smoke test covering progressive writes, interrupted file cleanup, atomic completion, and retry counters.
- No redesign of the existing interface.

## v0.2.0 — Released and tested on Windows
- Bounded retries, clearer error categories, failed/missing attachment counts, a Retry Failed action, and file-size validation.
- User confirmed that the new failure reporting worked after resuming 30+ monitored threads following the update.

## v0.1.1 — Released
- In-program update test from v0.1 successfully confirmed by the project owner.
- Version handling updated and graceful application shutdown preserved.

## v0.1 — Released
- 4chan JSON API parser, GitHub Releases updater, Windows Qt 5 portable packaging and compatible OpenSSL 3 runtime.
- Fixed clean application shutdown and verified actual Windows downloads.

---

## Historical upstream changelog

## Version 1.7.0
* 4chan url changes

## Version 1.6.3
* 4chan webm support

## Version 1.6.2
* 4chan plugin update for new image host domain

## Version 1.6.1
* Qt4 theme changes for Windows 

## Version 1.6.0
* Switch to Qt5

## Version 1.5.0
* Addressed instability issues
* More debug capabilities 

## Version 1.4.3
* SSL error on windows7 fixed
* Download status now working correctly

## Version 1.4.2
* Small bugfix release

## Version 1.4.1
* Added slideshow for the internal viewer
* Save path now supports formatting
 - e.g. C:/%b/%n will result in C:/wg/1234678
 - details on what formattings strings are supported are in the info dialog for each plugin
* Added proper debug system
* Added possibility to rebuild thumbnails for each thread and re-download a complete thread
* Some UI tweaks

## Version 1.4.0
* Added internal image viewer
* Added "Close to systray"
* Added Autosave function (every 10 minutes)

## Version 1.3.1
* Added checkbox to prevent closing of overview threads (e. g. boards.4chan.org/wg/2). -> Download ALL the 4chan!
* Hopefully fixed bug #3434272
* Added fancy progress bar for Win7 users

## Version 1.3.0
* Realised that one QNetworkAccessManager only opens up to 5 HTTP requests. If more simultaneous requests are desired an appropriate number of AccessManagers are created
* Added a plugin system to support others chans than 4chan
* Added plugin for 4chan
* Added plugin for 2chan (not thoroughly tested, because my japanese is not existent)
* The update files are now available from sourceforge in the webupdate folder
* The Qt system files can be updated
* Fixed issue that download stalls with one missing file
* Program menu is (nearly) back to the old version

## Version 1.2.0
* Added folder shortcuts - They can be defined in the options dialog
* Download manager now identifies as Opera (just for the statistics :D)
* Added thread overview widget for easier navigation
* Added a history of recently closed threads
* Replaced the toolbar with a normal menu to achieve the history function
* Added a lot of shortcuts:
 - Global
   Ctrl+A   Add single thread
   Ctrl+M   Add multiple threads
   Ctrl+O   Bring up thread overview widget
   Ctrl+S   Start all threads
   Ctrl+T   Stop all threads
   Ctrl+N   Configuration dialog (couldn't think of a better one)
   Ctrl+W   Close current tab
   Ctrl+Q   Close program
 - Only in thumbnail view
   Return/Enter Open image
   Delete       Delete image
   R            Reload image
* Preventing opening an already opened thread
* Not available images will not cause the thread to be closed

## Version 1.1.0
* Fixed program crash when a lots of tabs were open
* Some UI enhancements
* Added a dialogue to open multiple threads
 - It watches the clipboard, so copying a link or HTML source code with 4chan links will make them appear in this list
* To fix the mentioned code there is now only _one_ download thread. This means that the overall download with lots of images may appear slower,
  since it is only downloading the images tabwise and not all tabs at the same
* There is also only one thread which creates thumbnails, so this will also may appear slower than before. All CPU is used though.
* This leads to less memory usage, which is worth all the waiting though
* The thumbnail cache is enabled per default and it cannot be changed anymore due to a bug in the linux version
  - The linux version still complains about using QPixmap outside the UI thread, but I am not using it outside. So at the moment I will generously ignore that.
* I updated my Qt framework so you may download the complete package for the latest Qt libraries

## Version 1.0.0
* Added thumbnail cache
* Fixed bug 3302967

## Version 0.9.8
* Added support for proxies - I couldn't verify complete functionality with enabled proxy though, because all free proxies are banned

## Version 0.9.7
* Added Error Handler for image download
  - When an image is unavailable it will be rescheduled for download
  - This should fix bug 3271618
* Added buttons to open thread and download folder
* Double click on image now opens it
* Added a blacklist system
  - When deleting a file the url is blacklisted and will not be downloaded again
  - The blacklist can be turned of in the configuration dialog
  - To keep the list as small as necessary it will check at a given interval if the images are still available (not 404ed)

## Version 0.9.6
* Using different RegExp to retrieve files to get rid of "(...)" patterns - Thanks to Chris Moeller (kode54)
* Added define which makes Qt3 support unnecessary when compiling with Qt versions < 4.7 - Thanks to Salvador Parra Camacho
* Added ico file to resources that the application icon is displayed in taskbar
* Supporting top level URIs e. g. http://www.4chan.org/wg/1 will download images of all threads on that page
* Program now comes with an updater (au.exe) which replaces the executable once a new version is available (currently windows and linux only)

## Version 0.9.5
* Automatic reloading of partially received images
* Added freely configurable rescan timer values
* Added option to select if directory values should be saved within the session (inheriting directory value when creating a new tab)
* Added progress bar indicating how many thumbnails are in the queue to be rendered
* Fixed bug, where reloading of files with option "Save with original filename" lead to reloading wrong image
* Fixed wrong renaming of images when original filenames are equal due to elided text at 4chan post
* Some smaller bugfixes

- Storing of rescanning values have changed in settings.ini, so at the first start of this version the values have to be reset

## Version 0.9.4
* Moved thumbnails creation into own thread to not block the UI
* for each thread files with the same filename are saved with an appended counter
* bugfixes concerning the progress bar
* Bug fixed that savepath is empty after cancelling the folder selection dialog

## Version 0.9.3
* Saving state of each tab so that it automatically starts if it was running the last time
* Automatic checking if new version is available
* Changed Progressbar text content (always showed one more)

## Version 0.9.2
* Added Configuration Dialog with options to
 - resume from previous session
 - select default download folder
 - set size for thumbnail images
 - select rendering method for thumbnails (HQ uses a lot more CPU)
 - set the maximum number of concurrent downloads per tab (if you are experiencing corrupted downloads decreasing this number may help)
 - select tab position
* Settings can be changed while having downloads in progress, only thumbnail size is ignored once a thumbnail has been created

## Version 0.9.1
* Tabs are changing name now to see current download status even when tab is not active
* Option for preserving original filenames added

## Version 0.9
* Initial release
* Supports multiple tabs
* Automatic url pasting from clipboard when opening a new tab
* Rembering of savepath in one program instance (not persistent yet)
* Image preview and simple file operations
* When deleting a file it will not be downloaded again, despite of watching the image thread in a new tab
