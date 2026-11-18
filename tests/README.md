# Tests

Configure with Qt 6 Widgets and a matching compiler, then run `ctest --test-dir build-debug --output-on-failure` or the Release equivalent. Qt widget tests use the offscreen platform plugin. Keep the matching Qt and MinGW `bin` directories on `PATH` on Windows.

The suite covers core search, editing, storage, history, dialogs, main window, and controller behavior. `DictionaryV2` exercises legacy loading, version 2 save/load, escaped metadata, Unicode filenames, transactional failures, undo/redo, suggestions, and actual bundled seed loading. `AppControllerBoundary` also checks enriched entries and suggestion refresh after mutations.

Run `python scripts/validate_seed.py` separately for deterministic seed counts, uniqueness, UTF-8, enrichment, category coverage, and retention of all original terms. The validator does not replace the C++ load test.
