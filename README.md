# Tech Dictionary

Tech Dictionary is a planned Windows desktop English–Vietnamese IT dictionary for a
university Data Structures & Algorithms project. Version 0.1.0 is the project
foundation: it opens a styled Qt window, while dictionary features remain unimplemented.

## Academic objectives

The final project will group English words into 26 hash buckets by their first letter.
Each bucket will hold a custom singly linked list of words and Vietnamese meanings.
The team will implement and explain hashing, traversal, insertion, deletion, memory
management, searching, and file persistence rather than using a library container
for the core dictionary.

## Technology stack

- C++17
- Qt 6 Widgets
- CMake 3.16 or newer
- Custom hash table and singly linked lists (planned)

## Project structure

| Path | Purpose |
| --- | --- |
| `include/` | Shared dictionary API and application controller |
| `src/` | Entry point, controller, and future dictionary operations |
| `ui/` | Main window and future search and word dialog components |
| `resources/` | QSS theme and Qt resource manifest |
| `data/` | Reserved UTF-8 dictionary file |
| `tests/` | Test guidance; feature tests arrive with implementations |
| `docs/` | Architecture and coding standards |

`resources/resources.qrc` embeds `styles.qss` into the executable, so the theme
is available without relying on the process working directory.

## Build requirements

Install a C++17 compiler, CMake 3.16+, and a Qt 6 installation with the Widgets
component and its matching compiler kit. On Windows, use a Qt kit compatible with
your compiler (for example, MSVC Qt with MSVC or MinGW Qt with MinGW).

## Build and run

From the repository root:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/path/to/Qt/6.x.x/your-kit"
cmake --build build --config Debug
```

If Qt 6 is already discoverable through your environment, omit
`-DCMAKE_PREFIX_PATH`. With a single-configuration generator, run
`build/TechDictionary.exe`; with Visual Studio or another multi-configuration
generator, run `build/Debug/TechDictionary.exe`. The Qt runtime libraries must
also be available to the executable (for example, through a Qt-enabled shell or
deployment with Qt's `windeployqt`).

## Current UI and planned features

The current window shows a welcome message, Vietnamese UTF-8 text, and disabled
navigation and search controls marked as coming soon. Its content area expands
with the window. Planned features are word lookup and browsing, add/update/delete,
advanced search, settings, and text-file persistence. No dictionary algorithm is
linked into the v0.1.0 executable.

## Team responsibilities

| Member | Owned area |
| --- | --- |
| TV1 (leader) | App controller, CMake, integration, code review |
| TV2 | Hash buckets, linked lists, memory management |
| TV3 | Add, search, advanced search, search widget |
| TV4 | Update, delete, optional undo/redo, word dialog |
| TV5 | Main window, Qt layout, QSS design |
| TV6 | Dictionary display, file storage, sample data |

Each member should contribute C++ code. TV3 and TV4 own their Qt components in
addition to dictionary operations. The initial window is a minimal scaffold for
TV5 to refine.

## Development workflow

Keep the default branch buildable. After this skeleton is reviewed, create work
branches as needed: `feature/core`, `feature/search`, `feature/edit`, `feature/ui`,
and `feature/storage`. These branches do not exist yet. Open focused pull requests,
request a teammate's review, and merge only after the Qt build and relevant tests
pass. Add each feature's source files to `CMakeLists.txt` only when they contain
real implementations and their dependencies are ready. Follow
[`docs/CODING_STANDARDS.md`](docs/CODING_STANDARDS.md).

Before implementing the dictionary, the leader and module owners should settle
the invalid-word behavior for `hashWord`, case matching, duplicate handling, and
the text-file format. The declarations in [`include/dictionary.h`](include/dictionary.h)
provide ownership and result conventions without inventing algorithms for this
milestone.
