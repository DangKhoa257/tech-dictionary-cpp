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

For Release, use `build-release` and `-DCMAKE_BUILD_TYPE=Release`. Adjust the Qt path to your installed version. Qt 6 Widgets is required; configuration stops if it is missing. To deploy outside a Qt-enabled shell, use Qt's `windeployqt` and retain the `data/dictionary.txt` directory beside the executable.

## Using the application

The bundled `data/dictionary.txt` is loaded when no editable dictionary exists. The app keeps this seed read-only and saves changes to `QStandardPaths::AppDataLocation/dictionary.txt` for the current user. **File → Open** loads another UTF-8 TSV; **Save** writes the active editable file; **Save As** chooses a new path. Each line is `English<TAB>Vietnamese`. The app confirms unsaved changes before Open or Exit. A corrupt load leaves the current dictionary intact.

Search by English term, select a list item to see its meaning, or choose an A–Z range in the sidebar. Add, edit, and delete use the buttons in the main window. Edit → Undo/Redo use Ctrl+Z and Ctrl+Y. Changes made since the last save are marked in the window state.

## Project ownership

TV2 implemented hash and memory management; TV3 add/search and SearchWidget; TV4 update/delete, WordDialog, confirmation, and history; TV5 MainWindow and QSS; TV6 listing, storage, and seed data. TV1 implemented AppController, lifecycle, signal wiring, CMake integration, integration tests, and documentation. See [architecture](docs/ARCHITECTURE.md) for details.

## Limits

English keys must begin with ASCII A–Z and are matched without ASCII case sensitivity. Each operation is an exact term lookup; there is no prefix or fuzzy search. Undo/Redo history is in memory and resets when a dictionary file is opened or the application exits.
