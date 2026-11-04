# Coding standards

- Use C++17 and 4 spaces for indentation. Put opening braces on the same line.
- Use `camelCase` for variables and functions, `PascalCase` for structs and
  classes, `UPPER_SNAKE_CASE` for constants, and `snake_case` for filenames.
- Use English names for code symbols. Preserve UTF-8 for Vietnamese text and data.
- Use `#pragma once` in headers and avoid `using namespace std;` in headers.
- Prefer `const` where possible. Do not create unnecessary global variables.
- Keep the dictionary core independent of Qt. UI classes may use Qt types.
- Document pointer ownership, failure behavior, and assumptions in shared APIs.
- Match declarations in `include/dictionary.h` when implementing operations.
- Use branch names in `kebab-case` after the `feature/` or `chore/` prefix.
- Keep pull requests focused. Have another member review changes before merging.
