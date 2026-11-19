"""Build and audit the bundled V2 seed; never read or write personal dictionaries.

The original two-column terms are in dictionary_legacy.txt. The sole source
for their metadata is legacy_enrichment.tsv (English, part of speech, example,
category). v2_additions.tsv has category, English, Vietnamese, part of speech,
example. Run with --build to regenerate only data/dictionary.txt.
"""

import argparse
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEGACY = ROOT / "data/dictionary_legacy.txt"
ENRICHMENT = ROOT / "data/legacy_enrichment.tsv"
ADDITIONS = ROOT / "data/v2_additions.tsv"
SEED = ROOT / "data/dictionary.txt"
HEADER = "#TECHDICTIONARY\t2"
CATEGORIES = {
    "Basic Computing", "Programming", "Object-Oriented Programming",
    "Data Structures", "Algorithms", "Database", "Networking",
    "Operating Systems", "Cybersecurity", "Artificial Intelligence",
    "Machine Learning", "Data Science", "Software Engineering",
    "Cloud Computing", "Web Development",
}
PARTS = {
    "Noun", "Verb", "Adjective", "Adverb", "Noun phrase", "Verb phrase",
    "Abbreviation", "Other / Technical term",
}
PLACEHOLDERS = {"example sentence.", "todo", "tbd", "lorem ipsum"}


def read_utf8(path):
    # Decode strictly. A replacement character would hide a damaged source file.
    return path.read_bytes().decode("utf-8", errors="strict").splitlines()


def read_tsv(path, width):
    rows = []
    for number, line in enumerate(read_utf8(path), 1):
        fields = line.split("\t")
        if len(fields) != width:
            raise ValueError(f"{path.name} row {number}: expected {width} fields")
        rows.append(tuple(fields))
    return rows


def ascii_key(key):
    return (bool(key) and key == key.strip() and "A" <= key[0].upper() <= "Z"
            and key[0].isascii() and key[0].isalpha()
            and all(0x20 <= ord(character) <= 0x7E for character in key))


def folded(key):
    # Match Dictionary's ASCII-only case folding, not Unicode casefold().
    return key.translate(str.maketrans(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz"))


def source_rows():
    original = read_tsv(LEGACY, 2)
    metadata = read_tsv(ENRICHMENT, 4)
    additions = read_tsv(ADDITIONS, 5)
    if len(original) != 288 or len(metadata) != 288 or len(additions) != 300:
        raise ValueError("Expected exactly 288 legacy, 288 enrichment, and 300 addition rows")

    legacy_keys = [english for english, _ in original]
    if len({folded(key) for key in legacy_keys}) != len(legacy_keys):
        raise ValueError("Duplicate legacy English key")
    enrichment = {}
    enrichment_folded = set()
    for number, (english, part, example, category) in enumerate(metadata, 1):
        key = folded(english)
        if key in enrichment_folded:
            raise ValueError(f"Enrichment row {number}: duplicate key {english!r}")
        enrichment[english] = (part, example, category)
        enrichment_folded.add(key)
    missing = set(legacy_keys) - set(enrichment)
    extra = set(enrichment) - set(legacy_keys)
    if missing or extra:
        raise ValueError(f"Enrichment keys differ from legacy: missing={sorted(missing)}, extra={sorted(extra)}")

    rows = [(english, vietnamese, *enrichment[english])
            for english, vietnamese in original]
    rows.extend((english, vietnamese, part, example, category)
                for category, english, vietnamese, part, example in additions)
    return rows, legacy_keys


def escaped(value):
    if not value:
        return "\\e"
    return value.replace("\\", "\\\\").replace("\t", "\\t").replace("\n", "\\n").replace("\r", "\\r")


def unescape(value):
    if value == "\\e":
        return ""
    result = []
    index = 0
    while index < len(value):
        if value[index] != "\\":
            result.append(value[index])
        else:
            index += 1
            if index >= len(value) or value[index] not in "\\tnr":
                raise ValueError("Invalid TSV escape")
            result.append({"\\": "\\", "t": "\t", "n": "\n", "r": "\r"}[value[index]])
        index += 1
    return "".join(result)


def validate(rows, legacy_keys):
    if len(rows) != 588:
        raise ValueError(f"Expected 588 records, found {len(rows)}")
    if [row[0] for row in rows[:288]] != legacy_keys:
        raise ValueError("Original English keys or their order changed")

    seen = set()
    examples = set()
    counts = Counter()
    for number, row in enumerate(rows, 1):
        if len(row) != 5:
            raise ValueError(f"Row {number}: expected five fields")
        english, vietnamese, part, example, category = row
        if not ascii_key(english):
            raise ValueError(f"Row {number}: invalid English key {english!r}")
        key = folded(english)
        if key in seen:
            raise ValueError(f"Row {number}: duplicate key {english!r}")
        seen.add(key)
        if not vietnamese.strip() or vietnamese != vietnamese.strip():
            raise ValueError(f"Row {number}: empty or padded Vietnamese meaning")
        if any(ord(character) < 0x20 or ord(character) == 0x7F for character in vietnamese):
            raise ValueError(f"Row {number}: control character in Vietnamese meaning")
        if part not in PARTS:
            raise ValueError(f"Row {number}: unknown part of speech {part!r}")
        if category not in CATEGORIES:
            raise ValueError(f"Row {number}: unknown category {category!r}")
        if (not example.strip() or example != example.strip()
                or any(ord(character) < 0x20 or ord(character) == 0x7F for character in example)):
            raise ValueError(f"Row {number}: missing or malformed example")
        lowered_example = example.casefold()
        if (lowered_example in PLACEHOLDERS or lowered_example.startswith(
                ("todo:", "tbd:", "lorem ipsum"))):
            raise ValueError(f"Row {number}: placeholder example")
        if not example.endswith((".", "!", "?")):
            raise ValueError(f"Row {number}: example lacks sentence punctuation")
        if folded(english) not in folded(example):
            raise ValueError(f"Row {number}: example does not use {english!r}")
        if example.casefold() in examples:
            raise ValueError(f"Row {number}: repeated example")
        examples.add(example.casefold())
        counts[category] += 1
    if set(counts) != CATEGORIES:
        raise ValueError("Required categories are missing")
    return counts


def seed_rows():
    lines = read_utf8(SEED)
    if not lines or lines[0] != HEADER:
        raise ValueError("Missing version 2 header")
    rows = []
    for number, line in enumerate(lines[1:], 2):
        fields = line.split("\t")
        if len(fields) != 5:
            raise ValueError(f"Seed row {number}: expected five fields")
        try:
            rows.append(tuple(map(unescape, fields)))
        except ValueError as error:
            raise ValueError(f"Seed row {number}: {error}") from error
    return rows


def render_seed(rows):
    return (HEADER + "\n" + "\n".join(
        "\t".join(map(escaped, row)) for row in rows) + "\n").encode("utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", action="store_true")
    args = parser.parse_args()
    expected, legacy_keys = source_rows()
    counts = validate(expected, legacy_keys)
    if args.build:
        SEED.write_bytes(render_seed(expected))
    actual = seed_rows()
    validate(actual, legacy_keys)
    if actual != expected:
        raise ValueError("Bundled seed differs from curated sources; regenerate with --build")
    if SEED.read_bytes() != render_seed(expected):
        raise ValueError("Bundled seed has noncanonical escaping or line endings")
    print(f"Validated {len(actual)} unique complete entries; {len(legacy_keys)} legacy terms retained")
    for category in sorted(CATEGORIES):
        print(f"{category}: {counts[category]}")


if __name__ == "__main__":
    main()
