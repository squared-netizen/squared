# kit.{{project_name}}

A kit-authoring workspace. The deliverable is the cartridge under
`cartridge/`, not a binary.

Work in `cartridge/`. `Makefile` is editable but rarely needs editing; `mk/`
is generated and rewritten on update.

Run `make check` before claiming a change works. It validates the cartridge;
a manifest that parses is not the same as a manifest that is correct.

Do not add build steps that require the network. Generation and validation are
offline (Repository invariant §1.13).
