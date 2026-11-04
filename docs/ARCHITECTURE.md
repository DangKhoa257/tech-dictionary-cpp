# Architecture

Version 0.1.0 builds only the entry point, controller, and main window. The
dictionary modules below are planned integration points, not functioning features.

## Layers and ownership

1. **Qt UI:** `MainWindow` displays the shell. TV5 owns its layout and QSS. TV3
   will add `SearchWidget`; TV4 will add `WordDialog`. UI components should present
   results and collect input without owning linked-list nodes.
2. **Application controller:** `AppController` owns the main window and will connect
   UI actions to dictionary operations. TV1 owns integration. At present it only
   shows the window and runs the Qt event loop.
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
