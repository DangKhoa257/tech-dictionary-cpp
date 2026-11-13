# Architecture

## Ownership and flow

`src/main.cpp` creates `QApplication` and TV1's noncopyable `AppController`. The controller owns `Dictionary`, `DictionaryHistory`, and TV5's `MainWindow`; the window owns TV3's embedded `SearchWidget` and temporary TV4 `WordDialog` instances. The controller destructor calls TV2's `clearDictionary`. `Dictionary::buckets[26]` own singly linked `WordNode` chains. TV3/TV6 return non-owning pointers, which the controller converts to `QString` immediately. UI objects never retain node pointers.

| UI event | Controller action | Module |
| --- | --- | --- |
| SearchWidget `searchRequested` | `searchWord` | TV3 `findWord` |
| MainWindow `wordSelected` | Show copied meaning | TV3 `findWord` |
| MainWindow `filterRequested` | Refresh list for A–Z range | TV6 `listBucket` / `listAllWords` |
| MainWindow add/edit/delete requests | Dialog/confirmation, mutate and refresh | TV3 `addWord`; TV4 `updateWordMeaning` / `deleteWord` |
| Edit menu Undo/Redo | Restore history and refresh | TV4 `DictionaryHistory` |
| File menu Open/Save/Save As | Replace or persist dictionary | TV6 `loadDictionary` / `saveDictionary` |

TV1's `AppController` maps module results to `ControllerResult`, manages the active path and dirty state, and shows status/error messages. Qt strings and filenames cross the module boundary as UTF-8. The main window retains TV5's QSS and layout. The controller uses the exact six filter labels emitted by TV5.

## Data lifecycle

CMake copies the repository's UTF-8 TSV seed to `data/dictionary.txt` beside the build executable. At launch the controller first looks for a user-editable file at `QStandardPaths::AppDataLocation/dictionary.txt`; otherwise it loads the adjacent seed. Loading the seed sets the active save path to the user file, so ordinary Save does not overwrite the seed. Save As also rejects the seed path. A package must include the same adjacent `data/` directory. If neither file exists, the app starts empty and reports that state.

TV6 loads a complete TSV into a temporary dictionary before replacing current nodes. Invalid rows or I/O errors leave existing data untouched. TV6 saves via a sibling temporary file and replacement. The controller prompts Save/Discard/Cancel before Open or window close. A cancelled or failed save leaves the window open. Successful load clears Undo/Redo history and marks the dictionary clean.

Each successful add/update/delete captures before and after value snapshots and records a change in TV4 history. Undo/Redo reconstructs nodes, then the controller refreshes the list from scratch. Dirty state compares the live snapshot with the most recently loaded/saved snapshot, so undoing to the saved state removes the modified marker. History is not persisted across launches. If history recording fails after a dictionary mutation, the controller clears history and still refreshes the UI and marks data dirty; the operation remains successful with a warning message.

## Build and tests

Qt 6 Widgets is mandatory. The application target links TV2–TV6 source modules and all UI components. From a Qt/MinGW-enabled Windows shell:

```powershell
cmake -S . -B build-debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/mingw_64"
cmake --build build-debug -j 4
ctest --test-dir build-debug --output-on-failure
```

Repeat with `build-release` and `-DCMAKE_BUILD_TYPE=Release`. `AppControllerBoundary` uses a temporary directory inside the test build, exercises real operations and UI filter signals, and destroys controllers with populated dictionaries. Other CTest targets cover the original module boundaries.

## Limits

Search is exact and ASCII case insensitive for English keys. A term must start with A–Z; punctuation can follow. Invalid UTF-8 TSV is rejected. History is per session. The application has no in-app prefix/fuzzy search or import merge. TV2–TV6 retain ownership of their original implementations; TV1 owns this integration layer.
