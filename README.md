# Tech Dictionary

Tech Dictionary is a C++17 / Qt 6 Widgets desktop dictionary for English–Vietnamese IT terms. Terms live in 26 A–Z hash buckets, each backed by a singly linked list. The application supports search, browsing, A–Z filters, add/edit/delete, Undo/Redo, and UTF-8 TSV open/save.

## Build on Windows

Install Qt 6 Widgets, CMake 3.16+, and a compiler matching the Qt kit. For a Qt MinGW kit, add its `bin` directory and the matching MinGW `bin` directory to `PATH`, then run from the repository root:

```powershell
cmake -S . -B build-debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/mingw_64"
cmake --build build-debug -j 4
ctest --test-dir build-debug --output-on-failure
./build-debug/TechDictionary.exe
```

For Release, use `build-release` and `-DCMAKE_BUILD_TYPE=Release`. Adjust the Qt path to your installed version. Qt 6 Widgets is required; configuration stops if it is missing. A direct executable launch needs the matching Qt and MinGW `bin` directories on `PATH`. For a standalone test copy, place `TechDictionary.exe` and `data/dictionary.txt` together in a new directory, then run the matching kit's `windeployqt --release --compiler-runtime --dir <directory> <directory>/TechDictionary.exe`. This copies Qt DLLs, the Windows platform plugin and the compiler runtime beside the test executable; it does not change system directories.

For a complete local Release ZIP, run `./scripts/package_windows.ps1 -BuildDir build-release -QtRoot C:/Qt/6.12.0/mingw_64` from a PowerShell session with the matching MinGW and Qt `bin` folders on `PATH`. The script writes an ignored `dist/TechDictionary-v0.2.0-win64.zip` and verifies the executable, seed, Qt DLLs, platform plugin, and MinGW runtime files. The Windows GitHub Actions workflow builds Debug and Release, runs all seven CTest targets, validates the seed, packages Release, runs the packaged app's isolated `--ci-smoke` check, and uploads the ZIP plus a smoke screenshot. The smoke mode uses a temporary personal dictionary path and does not write user data. For a normal manual check, extract the ZIP and launch `TechDictionary.exe` without the smoke flag.

## Using the application

The bundled `data/dictionary.txt` is loaded when no editable dictionary exists. The app keeps this seed read-only and saves changes to `QStandardPaths::AppDataLocation/dictionary.txt` for the current user. **File → Open** loads a legacy two-column UTF-8 TSV or a version 2 dictionary; **Save** writes the active editable file; **Save As** chooses a new path. The app confirms unsaved changes before Open or Exit. A corrupt load leaves the current dictionary intact.

The version 2 bundled seed contains 588 unique terms. All 288 original terms remain, and the original seed is preserved verbatim in `data/dictionary_legacy.txt`. The 300 new terms have a part of speech, English example, and category. Fifteen original terms have also been enriched. Run `python scripts/validate_seed.py` to check counts, uniqueness, categories, UTF-8, and legacy retention. The curated source for new terms is `data/v2_additions.tsv`.

Version 2 files begin with `#TECHDICTIONARY<TAB>2`. Each following physical row has five tab-separated fields: English term, Vietnamese meaning, part of speech, English example, and category. Empty optional fields use `\e`. Backslash, tab, newline, and carriage return within version 2 fields use `\\`, `\t`, `\n`, and `\r` escapes. Invalid escapes, duplicate keys, bad UTF-8, or malformed rows reject the entire load. Saving a nonempty dictionary writes version 2 atomically, including after a legacy load. Opening a legacy file does not rewrite it, and startup migration never overwrites a personal dictionary.

If the user dictionary cannot be read at startup, a one-time recovery dialog offers another file, the bundled seed, or an empty dictionary. The unreadable file is preserved. After seed or empty recovery, Save asks for a path; choosing to replace the unreadable original requires an additional explicit confirmation. If both files are missing, the app starts empty and remains usable.

Type an English prefix to see up to eight live suggestions. Select a suggestion with the arrow keys and Enter or with the mouse; Escape closes the popup. Select a list item to see its meaning, part of speech, English example, and category when available, or choose an A–Z range in the sidebar. Add, edit, and delete use the buttons in the main window. Edit → Undo/Redo use Ctrl+Z and Ctrl+Y. Changes made since the last save are marked in the window state.

## Project ownership

TV2–TV6 implemented the original modules. TV1 owns the version 2 feature extension: metadata in the shared WordNode contract and history snapshots, versioned storage, autocomplete integration, seed expansion, validation, tests, and documentation. The 26-bucket hash table and singly linked lists remain the primary dictionary store. See [architecture](docs/ARCHITECTURE.md) for details.

## Limits

English keys must begin with ASCII A–Z and are matched without ASCII case sensitivity. Suggestions use prefix matching, while exact lookup and editing still use whole terms. There is no fuzzy search. Undo/Redo history is in memory and resets when a dictionary file is opened or the application exits. Legacy entries without metadata show their existing meaning until enriched by the user.
