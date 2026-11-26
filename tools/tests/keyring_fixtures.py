"""Hand-written keyring fixtures shared by the publish-tooling tests.

Nothing here is derived from the code under test: the values are the decoded
public keys of the two committed files and the canonical document written out
literally, so a change in the tools cannot make an expectation follow it.
"""

from __future__ import annotations

import pathlib

KEYRING = pathlib.Path(__file__).resolve().parents[2] / "client" / "keyring"

TEST_XY = (
    "d7027588352422870b83a9a154ba0aaf105b8b5c191bf5a138562b369428"
    "8676e11271cf055e03e452a45448e825ec125bec1eea94fc06ae6e4d996ec6fac4d7"
)
PROD_XY = (
    "b3cae3d4a15f2e81719b9373b1b37a7050d829f563354868f5b98063bd98608"
    "ce32d037297fbf79e94ee9e2b78b6be25ca692d234382ac731f70ea065e397de0"
)
TEST_FINGERPRINT = "ca7795786820dd5d3d669fb3321b3e11bdafc1cce7d39c4fd59f52e7ef110aea"
PROD_FINGERPRINT = "58bb4787245691066105914d20ff3da5da2cb8677b13e943a40f6a73bcc277a8"

EMPTY_FLOOR = '{\n  "format": "axlic-trust-floor",\n  "version": 1,\n  "keys": []\n}\n'


def floor_document(key_id: str, xy: str) -> str:
    """The canonical single-entry document, written out by hand."""
    return (
        "{\n"
        '  "format": "axlic-trust-floor",\n'
        '  "version": 1,\n'
        '  "keys": [\n'
        "    {\n"
        f'      "key_id": "{key_id}",\n'
        f'      "public_key_xy_hex": "{xy}"\n'
        "    }\n"
        "  ]\n"
        "}\n"
    )


def keyed_exe(key_id: str = "license-prod", xy: str = PROD_XY) -> bytes:
    """Bytes shaped like an executable that embeds the given environment's floor."""
    return b"MZ" + bytes(64) + floor_document(key_id, xy).encode() + bytes(64)
