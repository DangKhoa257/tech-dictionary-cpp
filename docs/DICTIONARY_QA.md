# Dictionary content QA (V2)

The bundled seed is generated from 288 legacy records and 300 curated additions. All 588 English keys are unique under the application's ASCII case-insensitive matching rule. Each record has a nonempty Vietnamese meaning, a valid part of speech, an original English example sentence, and one of 15 categories. `data/legacy_enrichment.tsv` is the single authority for legacy metadata; its keys must match `data/dictionary_legacy.txt` exactly. The build script rejects missing, extra, or duplicate metadata records.

The original English keys and their order were preserved. Two legacy Vietnamese meanings were corrected: **Prompt** now describes an AI prompt as a *lời nhắc* rather than only a command, and **Socket** is described as a network communication endpoint rather than a literal socket. No user dictionary file was read or changed.

## Editorial pass

Every legacy row received a term-specific example and part of speech. The pass distinguished verbs (`search`, `Delete`, `Merge`), adjectives (`Binary`, `Digital`, `Peer-to-Peer`), abbreviations (`CPU`, `DNS`, `IoT`), multiword noun phrases, and proper technical names. Ambiguous entries were aligned with a specific category: **Token** uses the authentication sense in Cybersecurity; **Runtime** uses the time-duration sense in Algorithms; **Heap** uses the data-structure sense.

All 300 additions were reviewed. The following confirmed issues were corrected while preserving their English keys:

| Entry | Correction |
| --- | --- |
| Rollback | Uses the noun sense with a grammatical transaction example. |
| NAT | Vietnamese meaning says network address *translation/conversion* rather than compilation. |
| Statistical power | Uses the standard statistical sense of test power. |
| Heuristic | Meaning covers a rule of thumb, rather than only an evaluation function. |
| Salt | Meaning specifies the random value used when hashing a password. |
| Reasoning model | Example no longer promises a particular internal checking procedure. |

Related but distinct terms were retained, such as **Disjoint set** and **Union-find**, **CPU** and **Central Processing Unit**, and **Machine Learning** and **Deep Learning**. There are no case-insensitive duplicate English keys or repeated example sentences. The examples were written for this project; no dictionary definitions or example sentences were copied from a third-party dictionary.

Two entries merit optional human editorial review of scope and Vietnamese style: **OpenAI** is a proper organization name rather than a general technical concept, and **Agentic workflow** is an evolving AI term. Their existing meanings and keys were retained. This pass checked technical usage and internal consistency; it was not an independent external fact check of all 588 definitions.

## Category distribution

| Category | Terms |
| --- | ---: |
| Algorithms | 44 |
| Artificial Intelligence | 29 |
| Basic Computing | 61 |
| Cloud Computing | 27 |
| Cybersecurity | 36 |
| Data Science | 32 |
| Data Structures | 32 |
| Database | 38 |
| Machine Learning | 36 |
| Networking | 52 |
| Object-Oriented Programming | 29 |
| Operating Systems | 28 |
| Programming | 59 |
| Software Engineering | 46 |
| Web Development | 39 |
| **Total** | **588** |

## Reproduction

Run `python scripts/validate_seed.py --build` to regenerate only `data/dictionary.txt`. Run `python scripts/validate_seed.py` to check the committed seed against both curated sources, and `python -m unittest discover -s tests -p test_seed_validation.py` to exercise rejection of missing metadata, malformed TSV, invalid UTF-8, bad escapes, placeholder examples, duplicate keys, and unknown labels. The C++ `DictionaryV2` test loads the actual seed, verifies every legacy key and all 588 metadata records, and round-trips the complete dictionary through Save/Load. The Windows workflow runs these checks along with all existing CTest targets and a packaged GUI smoke test.
