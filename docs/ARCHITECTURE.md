# Architecture

Version 0.1.0 builds only the entry point, controller, and main window. The
dictionary modules below are planned integration points, not functioning features.

## Layers and ownership

1. **Qt UI:** `MainWindow` displays the shell. TV5 owns its layout and QSS. TV3
   will add `SearchWidget`; TV4 will add `WordDialog`. UI components should present
   results and collect input without owning linked-list nodes.
2. **Application controller:** `AppController` owns the main window and an initially
   empty `Dictionary`. TV1 owns integration. It currently launches the window,
   validates empty input, and returns explicit unavailable results for operations
   whose implementations are not linked.
3. **Dictionary operations:** `include/dictionary.h` declares the shared API. TV2
   owns hashing and memory cleanup; TV3 owns add/search; TV4 owns update/delete;
   TV6 owns listing and storage. Operation `.cpp` files are excluded from CMake
   until implemented.
4. **Hash table and linked lists:** `Dictionary::buckets[26]` holds heads of 26
   singly linked lists. A node stores English and Vietnamese UTF-8 strings plus a
   `next` pointer. The dictionary owns the nodes. Returned node pointers are
   non-owning and become invalid after deletion or clearing.
5. **File storage:** TV6 will read and write `data/dictionary.txt` or a path chosen
   by the user. The file is empty until the storage format is agreed upon.

## Communication

The UI communicates through the controller. The controller calls the dictionary
API and translates results into values or messages the UI can display. Dictionary
operations should not depend on Qt, allowing them to be tested separately.
Storage uses the same dictionary model; it should report errors to the controller,
which decides what to show in the UI.

For example, after search is implemented, entering **Algorithm** in `SearchWidget`
will send the term to `AppController`. The controller will call `findWord`. The
search implementation will derive the A bucket and traverse its linked list. It
will return a non-owning pointer or `nullptr`; the controller will display the
Vietnamese meaning or a not-found message. None of this flow is active in v0.1.0.

## Contracts to finalize during integration

The team needs one shared rule for invalid hash input, case sensitivity, duplicate
English entries, and the storage file format. `hashWord` returns an index, so its
invalid-input behavior needs special care before any caller relies on it. Keep
all files UTF-8. Add real tests with each implementation and add its source to
CMake only when it is ready to link.

## TV1 integration contract

`AppController` is the application boundary. UI components will send `QString`
values to its public methods. Each method returns a `ControllerResult` with a
`ControllerStatus` and a user-facing message. Search will also set `vietnamese`
on success. `InvalidInput` means required input is empty or whitespace;
`Unavailable` means the implementation is not linked; `NotFound`, `Failed`, and
`Success` are reserved for completed modules. A valid request currently returns
`Unavailable`, never a fabricated success. The controller must convert `QString`
to/from UTF-8 `std::string` when calling the dictionary API.

| Connection | Initiator and input | Expected output | Error reporting | Owner |
| --- | --- | --- | --- | --- |
| UI → controller | TV3 search widget sends English text; TV4 word dialog sends English and Vietnamese text; TV5 main window presents navigation | A `ControllerResult` for each request | Controller returns `InvalidInput` or `Unavailable` now; later `NotFound`/`Failed` as appropriate | TV3, TV4, TV5 for UI; TV1 for wiring |
| Controller → TV2 core | TV1 passes the owned `Dictionary` to hashing and cleanup as needed | Valid bucket behavior and released nodes | Core reports or documents invalid hash input; controller maps failures to a result | TV2 core; TV1 integration |
| Controller → TV3 add/search | English text and, for add, Vietnamese meaning in UTF-8 | `addWord` boolean or non-owning `findWord` pointer; prefix search needs a separately agreed API | `false`/`nullptr` is mapped to a result after invalid, duplicate, and not-found rules are agreed | TV3 implementation; TV1 integration |
| Controller → TV4 edit | English text and optional new Vietnamese meaning in UTF-8 | `updateWordMeaning`/`deleteWord` boolean | `false` is mapped to `NotFound` or `Failed` under the agreed contract | TV4 implementation; TV1 integration |
| Controller → TV6 display/storage | Owned dictionary and a file path for load/save; list requests use bucket index or all entries | Non-owning word pointers from listing; load/save boolean | Invalid path is rejected by controller; file errors become `Failed` once storage can distinguish them | TV6 implementation; TV1 integration |
| Controller → UI refresh | Successful mutation or load | TV5/TV3 display receives updated values, never ownership of nodes | Controller presents operation errors without refreshing stale data | TV1 wiring; TV5/TV3 presentation |

The controller is non-copyable because `Dictionary` contains owning raw pointers.
No current operation creates nodes, so the initial zeroed dictionary is safe at
shutdown. **Before any node-producing module is connected**, TV2 must implement
`clearDictionary`, TV1 must link that module and call cleanup when the controller
is destroyed, and integration tests must cover shutdown after insertion/loading.
Do not let the UI keep `WordNode*` after a mutation or cleanup.

Add `dictionary_core.cpp` only when hashing and cleanup are implemented. Add
`dictionary_search.cpp` with TV3's completed functions, `dictionary_edit.cpp`
with TV4's completed functions, and `dictionary_display.cpp` and
`dictionary_storage.cpp` with TV6's completed functions. Add `search_widget.cpp`
and `word_dialog.cpp` after their classes and request/response signals are ready.
Review unresolved symbols and ownership before each CMake change. The current
`MainWindow` has no request signals or methods for displaying results, so no UI
connection can be made without TV5's interface changes.
