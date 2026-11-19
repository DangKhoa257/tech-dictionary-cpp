"""Negative and reproducibility checks for the curated bundled dictionary."""

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from scripts import validate_seed as seed


class SeedValidationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.rows, cls.legacy_keys = seed.source_rows()
        cls.temp_root = seed.ROOT / "dist"
        cls.temp_root.mkdir(exist_ok=True)

    def reject_field(self, row_number, field_number, replacement, message):
        rows = list(self.rows)
        changed = list(rows[row_number])
        changed[field_number] = replacement
        rows[row_number] = tuple(changed)
        with self.assertRaisesRegex(ValueError, message):
            seed.validate(rows, self.legacy_keys)

    def test_all_sources_match_canonical_seed(self):
        self.assertEqual((288, 300, 588),
                         (len(self.legacy_keys), len(self.rows) - len(self.legacy_keys),
                          len(self.rows)))
        self.assertEqual(seed.seed_rows(), self.rows)
        self.assertEqual(seed.SEED.read_bytes(), seed.render_seed(self.rows))
        self.assertEqual(len(seed.validate(self.rows, self.legacy_keys)), 15)

    def test_missing_fields_duplicates_and_legacy_loss(self):
        self.reject_field(0, 2, "", "unknown part of speech")
        self.reject_field(287, 3, "", "missing or malformed example")
        self.reject_field(300, 1, "  ", "Vietnamese meaning")
        self.reject_field(300, 4, "Unknown", "unknown category")
        self.reject_field(300, 2, "Thing", "unknown part of speech")
        self.reject_field(300, 0, "Algorithm", "duplicate key")
        self.reject_field(0, 0, "Other", "Original English keys")

    def test_bad_keys_and_placeholder_examples(self):
        self.reject_field(288, 0, "1Algorithm", "invalid English key")
        self.reject_field(288, 0, "Algorithm ", "invalid English key")
        self.reject_field(0, 3, "Example sentence.", "placeholder example")
        self.reject_field(0, 3, "TODO", "placeholder example")
        self.reject_field(0, 3, "The algorithm lacks punctuation", "sentence punctuation")
        self.reject_field(0, 3, "The system sorts values.", "does not use")

    def test_real_todo_context_is_not_treated_as_placeholder(self):
        rows = list(self.rows)
        rows[0] = ("Algorithm", rows[0][1], rows[0][2],
                   "The algorithm tracks TODO comments in the source file.", rows[0][4])
        self.assertEqual(len(seed.validate(rows, self.legacy_keys)), 15)

    def test_enrichment_keys_are_exact_and_unique(self):
        original = seed.ENRICHMENT.read_text(encoding="utf-8")
        with tempfile.TemporaryDirectory(dir=self.temp_root) as directory:
            target = Path(directory) / "metadata.tsv"
            lines = original.splitlines()
            for replacement, message in (
                ("Unknown\tNoun\tThe unknown term appears.\tAlgorithms", "Enrichment keys differ"),
                (lines[1], "duplicate key"),
                (lines[0].replace("Algorithm", "algorithm"), "Enrichment keys differ"),
            ):
                changed = lines.copy()
                changed[0] = replacement
                target.write_text("\n".join(changed) + "\n", encoding="utf-8")
                with patch.object(seed, "ENRICHMENT", target):
                    with self.assertRaisesRegex(ValueError, message):
                        seed.source_rows()
            target.write_text("\n".join(lines[:-1]) + "\n", encoding="utf-8")
            with patch.object(seed, "ENRICHMENT", target):
                with self.assertRaisesRegex(ValueError, "Expected exactly"):
                    seed.source_rows()

    def test_invalid_utf8_tsv_shape_and_escape(self):
        with tempfile.TemporaryDirectory(dir=self.temp_root) as directory:
            target = Path(directory) / "seed.tsv"
            for content, message in (
                (b"\xff", UnicodeDecodeError),
                (b"#TECHDICTIONARY\t2\nBroken\trow\n", ValueError),
                (b"#TECHDICTIONARY\t2\nBroken\tvalue\tNoun\tbad\\x\tProgramming\n", ValueError),
            ):
                target.write_bytes(content)
                with patch.object(seed, "SEED", target):
                    with self.assertRaises(message):
                        seed.seed_rows()


if __name__ == "__main__":
    unittest.main()
