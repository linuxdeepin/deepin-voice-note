# dangerous-tags-script

Category: degradation

Note: libxml2 HTMLparser treats `<script>` as CDATA-special content and does
not surface it as a regular element node, so `dangerous-html-node` is NOT
emitted for `<script>` (unlike `<iframe>`/`<object>`/`<embed>` which do trigger
the warning — see `dangerous-tags-iframe`). The script content is silently
dropped; only the safe paragraph survives. The envelope golden (script dropped,
"safe" preserved as paragraph) is correct; warnings expected to be empty.

input.html is a sanitized placeholder; no real paths/voice content/accounts/names.
