# docs/

Generated documentation for this project: output from doxygen, LDoc, and any other tool that writes docs. Tools write here and you read it here. Hand-written docs live next to the code they describe:

| where | what |
|---|---|
| `docs/` | generated: API reference and anything a tool produces |
| `sq_app/docs/` | yours, hand-written: design notes, decisions, how-tos for this app |
| `squared/docs/` | the framework's programmer and developer docs, vendored with it |
| `sq_android/docs/` | platform notes, if you keep any |

To bundle all of them into one zip, for reading elsewhere or sharing off-device:

    tools/docs-archive.sh --list                      # what would go in
    tools/docs-archive.sh ~/storage/shared/Download   # write the archive there

This file is yours (ownership class: user). The generator wrote it once and won't touch it again.
