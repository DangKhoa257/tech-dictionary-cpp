# TV4 fixed files

This package contains the TV4 changes prepared against the PR #6 requirements
and the current `main` contracts inspected on 2026-11-11.

## File placement

- `src/dictionary_edit.cpp`
- `ui/word_dialog.h`
- `ui/word_dialog.cpp`
- `ui/delete_confirm.h`
- `ui/delete_confirm.cpp`
- `tests/dictionary_edit_test.cpp`
- `CMakeLists.txt` (minimal TV4 target/test additions to the current main file)

Do not copy `include/dictionary.h`, `src/dictionary_core.cpp`,
`src/dictionary_search.cpp`, or other team-owned modules from this package:
they are intentionally not included or reimplemented.

## Behavior changes

- TV4 production source defines only `updateWordMeaning` and `deleteWord`.
- It calls TV2's `hashWord()` and checks the returned index before bucket access.
- English matching is ASCII case-insensitive; stored English keys are not rewritten.
- Vietnamese UTF-8 meanings are assigned unchanged.
- Empty or whitespace-only meanings are rejected.
- Delete unlinks and frees only the first matching node.
- `WordDialog` validates inputs and emits `saveRequested`, but does not close itself.
  The caller/controller must show success and call `accept()` only after the
  dictionary operation succeeds. On failure it can call `setValidationMessage()`.
- Edit mode makes the English key read-only.
- Delete confirmation defaults to No.

## Tests

The TV4 tests use the real TV2 `src/dictionary_core.cpp` implementation and
construct test nodes directly. They do not duplicate TV2/TV3/TV6 production
algorithms.

Run from the repository root after applying the files:

    cmake -S . -B build
    cmake --build build --target DictionaryEditTests TV4DictionaryEdit TV4WordDialog
    ctest --test-dir build --output-on-failure

The CMake configure step still requires Qt6 Widgets because the existing project
requires it globally. A direct non-Qt test command is also possible:

    g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude \
        tests/dictionary_edit_test.cpp src/dictionary_core.cpp \
        src/dictionary_edit.cpp -o dictionary_edit_test
    ./dictionary_edit_test

## Integration boundary

`WordDialog::saveRequested` is intentionally only a request. TV1/TV5 must connect
it to the controller/UI workflow. The connected handler should call the controller
or dictionary operation, then:
- on success: call `dialog->accept()`;
- on failure: call `dialog->setValidationMessage(...)` and leave it open.

Do not wire this into `main_window.cpp` or `app_controller.cpp` from TV4.


## Undo/Redo history (added as an optional module)

Files:
- `include/dictionary_history.h`
- `src/dictionary_history.cpp`

`DictionaryHistory` stores value-only snapshots (English/Vietnamese strings,
bucket by bucket). It does not own dictionary nodes between operations.
After any successful mutation, the caller must capture the state before and
after the operation and pass both snapshots to `recordChange()`.

Example integration pattern:

```cpp
auto before = DictionaryHistory::capture(dictionary);
if (addWord(dictionary, english, vietnamese)) { // TV3 operation
    history.recordChange(std::move(before),
                         DictionaryHistory::capture(dictionary));
}

auto beforeEdit = DictionaryHistory::capture(dictionary);
if (updateWordMeaning(dictionary, english, newMeaning)) {
    history.recordChange(std::move(beforeEdit),
                         DictionaryHistory::capture(dictionary));
}

auto beforeDelete = DictionaryHistory::capture(dictionary);
if (deleteWord(dictionary, english)) {
    history.recordChange(std::move(beforeDelete),
                         DictionaryHistory::capture(dictionary));
}

// UI actions:
history.undo(dictionary);
history.redo(dictionary);
```

Important integration note: TV1/TV5 must connect Undo/Redo menu or toolbar
actions to the same `DictionaryHistory` instance and refresh the displayed
dictionary after a successful undo/redo. TV3's add operation must also be
wrapped as shown; otherwise additions will not enter the history. The history
module restores complete bucket snapshots, preserving bucket/list order and
Vietnamese UTF-8 strings. It does not persist history across application restarts.
