"""Build and validate the version 2 seed without touching personal dictionaries.

Run with --build after editing the curated additions or legacy seed. Without that
flag this script only validates the committed bundled dictionary.
"""
import argparse
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEGACY = ROOT / "data/dictionary_legacy.txt"
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
LEGACY_ENRICHMENT = {
    "Algorithm": ("Noun", "The algorithm sorts the array in ascending order.", "Algorithms"),
    "Compiler": ("Noun", "The compiler translates source code into machine instructions.", "Programming"),
    "Class": ("Noun", "The class defines the fields and methods of each object.", "Object-Oriented Programming"),
    "Stack": ("Noun", "The stack removes the most recently added item first.", "Data Structures"),
    "Database": ("Noun", "The application stores customer records in a database.", "Database"),
    "Router": ("Noun", "The router forwards packets between the two networks.", "Networking"),
    "Kernel": ("Noun", "The kernel manages memory and hardware access.", "Operating Systems"),
    "Firewall": ("Noun", "The firewall blocks traffic from an untrusted address.", "Cybersecurity"),
    "Artificial Intelligence": ("Noun phrase", "Artificial intelligence helps classify images of damaged roads.", "Artificial Intelligence"),
    "Machine Learning": ("Noun phrase", "Machine learning detects patterns in historical sales data.", "Machine Learning"),
    "Data Analysis": ("Noun phrase", "Data analysis revealed a seasonal increase in traffic.", "Data Science"),
    "Git": ("Other / Technical term", "Git records the changes made to each source file.", "Software Engineering"),
    "Cloud Computing": ("Noun phrase", "Cloud computing provides servers on demand.", "Cloud Computing"),
    "HTML": ("Abbreviation", "HTML marks the headings and links on the page.", "Web Development"),
    "CPU": ("Abbreviation", "The CPU executes the program's instructions.", "Basic Computing"),
}


def read_utf8(path):
    return path.read_text(encoding="utf-8", errors="strict").splitlines()


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
                raise ValueError("Invalid escape")
            result.append({"\\": "\\", "t": "\t", "n": "\n", "r": "\r"}[value[index]])
        index += 1
    return "".join(result)


def legacy_rows():
    rows = []
    for number, line in enumerate(read_utf8(LEGACY), 1):
        parts = line.split("\t")
        if len(parts) != 2:
            raise ValueError(f"Legacy row {number}: expected two fields")
        part, example, category = LEGACY_ENRICHMENT.get(parts[0], ("", "", ""))
        rows.append((parts[0], parts[1], part, example, category))
    return rows


def addition_rows():
    rows = []
    for number, line in enumerate(read_utf8(ADDITIONS), 1):
        parts = line.split("\t")
        if len(parts) != 5:
            raise ValueError(f"Addition row {number}: expected five fields")
        category, english, vietnamese, part, example = parts
        rows.append((english, vietnamese, part, example, category))
    return rows


def validate(rows):
    seen = set()
    counts = Counter()
    for number, (english, vietnamese, part, example, category) in enumerate(rows, 1):
        key = english.strip().lower()
        if not english or english != english.strip() or not english[0].isascii() or not english[0].isalpha():
            raise ValueError(f"Row {number}: invalid English key {english!r}")
        if key in seen:
            raise ValueError(f"Row {number}: duplicate key {english!r}")
        seen.add(key)
        if not vietnamese.strip() or vietnamese != vietnamese.strip():
            raise ValueError(f"Row {number}: empty or padded Vietnamese meaning")
        if category:
            if category not in CATEGORIES or part not in PARTS or not example.strip():
                raise ValueError(f"Row {number}: invalid enrichment")
            counts[category] += 1
        elif part or example:
            raise ValueError(f"Row {number}: incomplete enrichment")
    legacy_keys = {row[0].lower() for row in legacy_rows()}
    if not legacy_keys.issubset(seen):
        raise ValueError("Legacy terms were lost")
    if len(rows) < 550 or set(counts) != CATEGORIES:
        raise ValueError(f"Insufficient count or category coverage: {len(rows)}")
    return counts


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", action="store_true")
    args = parser.parse_args()
    if args.build:
        rows = legacy_rows() + addition_rows()
        validate(rows)
        SEED.write_text(HEADER + "\n" + "\n".join("\t".join(map(escaped, row)) for row in rows) + "\n", encoding="utf-8")
    lines = read_utf8(SEED)
    if not lines or lines[0] != HEADER:
        raise ValueError("Missing version 2 header")
    rows = []
    for number, line in enumerate(lines[1:], 2):
        parts = line.split("\t")
        if len(parts) != 5:
            raise ValueError(f"Row {number}: expected five fields")
        rows.append(tuple(map(unescape, parts)))
    counts = validate(rows)
    print(f"Validated {len(rows)} unique entries; {len(legacy_rows())} legacy terms retained")
    for category in sorted(CATEGORIES):
        print(f"{category}: {counts[category]}")


if __name__ == "__main__":
    main()
