# utf8proc 2.11.3

Upstream: https://github.com/JuliaStrings/utf8proc/tree/v2.11.3
API: https://juliastrings.github.io/utf8proc/doc/utf8proc_8h.html

Unmodified release files, fetched from the upstream v2.11.3 tag. The terminal uses UTF-8 decoding, Unicode 17 grapheme boundaries, character categories and cell widths. See LICENSE.md for the MIT and Unicode data licenses. No network fetch occurs at configure/build time.

SHA-256:

- utf8proc.c: `edf80118fd34796bcabef4567cc684adebc7058f2afe8cfdc11b7a78e383ed84`
- utf8proc.h: `a4e498b7392c383cf3b22e662da21e0b48f1264806235d87b5d8bd166232f658`
- utf8proc_data.c: `950e549dbfc853c4304425f3af1875e72fa9fc9697c273c763400c2da4e380a7`
- LICENSE.md: `3b510150d34f248a221bb88e1d811238d6c6c18b51231822c42974c39bb07256`

Earthcall lowers each grapheme to the maximum constituent width, with emoji-presentation, flag and keycap sequences occupying two cells. East Asian ambiguous characters use one cell. Terminal font/emulator choices can differ; native-terminal acceptance remains separate from this deterministic layout contract.

Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-14T22:30:39-07:00.
