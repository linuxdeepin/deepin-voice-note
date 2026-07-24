# TTP-014 HTML Migration Fixtures

File-driven golden regression data for `MigrationHtmlConverter::convert()`.

## Layout

Each fixture is a subdirectory:

| File | Purpose |
| --- | --- |
| `input.html` | Sanitized legacy HTML fed to `convert()` |
| `golden.json` | Expected envelope (deep-equal) |
| `warnings.json` | Expected warnings `[{code, path}]` |
| `errors.json` | Expected errors `[{code, path}]` (defaults to `[]`) |
| `README.md` | Per-fixture notes |

The harness (`tests/src/importolddata/tiptapmigration/ut_migrationhtmlfixtures.cpp`)
auto-discovers every subdirectory of this folder via `QDir`; drop in a new
fixture and it is picked up with no code or CMake change.

## Coverage (36 fixtures)

### Non-degradation (9)

`plain-text-multiline`, `formatted-marks`, `heading-blockquote`, `image-http`,
`image-relative`, `voice-normal`, `list-nested`, `empty-doc`, `real-combo`.

### Degradation (27)

| Dimension | Fixtures |
| --- | --- |
| dangerous-tags | `dangerous-tags-script`, `dangerous-tags-iframe` |
| unknown-block | `unknown-block-article`, `unknown-block-pre` |
| unknown-inline | `unknown-inline-display`, `unknown-inline-transparent` |
| dangerous-attribute | `dangerous-attribute-onclick` |
| dangerous-link | `dangerous-link`, `dangerous-link-javascript` |
| unsupported-style | `unsupported-style-margin`, `unsupported-style-multiple` |
| invalid-style-value | `invalid-style-value` |
| text-align | `text-align-downgrade` |
| base64-image | `base64-image` |
| unsafe-image-src | `unsafe-image-src-javascript`, `unsafe-image-src-network` |
| missing-image-src | `missing-image-src-empty`, `missing-image-src-absent` |
| voice (R1 inline literals) | `voice-generated-id`, `voice-negative-size`, `voice-oversized-size`, `voice-unexpected-type` |
| voice (codes.h) | `voice-missing-size`, `voice-missing-jsonkey`, `voice-invalid-jsonkey` |
| orphan-list-item | `orphan-list-item` |
| parse-failed (R4 fallback) | `parse-malformed-tolerated` |

Warning codes are pinned against `migrationhtmlcodes.h` (single source of
truth). The five voice codes `generated-voice-id`, `negative-voice-size`,
`normalized-voice-path`, `oversized-voice-size`, `unexpected-voicebox-type` are
**inline string literals** in `migrationhtmlconverter.cpp` (not yet in the
header — R1 gap); fixtures pin the actual emitted values.

## Golden capture / refresh

Goldens were hand-traced from `migrationhtmlconverter.cpp` at baseline
`6514340`. Because they must equal `convert()` output exactly, lock or refresh
them from the real converter before relying on them:

```sh
MIGRATION_HTML_FIXTURES_REGEN=1 ./tests/deepin-voice-note-test \
    --gtest_filter=UT_MigrationHtmlFixtures.*
```

This overwrites `golden.json`/`warnings.json`/`errors.json` in place from the
actual `convert()` output. Review the diff, then commit. The default (verify)
mode compares actual vs golden and reports a `toCompactJson` diff on mismatch.

## Sanitization (D7B)

All `input.html` use neutral placeholders — no real paths, voice content,
accounts, names, or real filenames. No binary content (D9A).
