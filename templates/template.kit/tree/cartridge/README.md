# kit.{{project_name}}

## `tree/` is payload

Everything under `tree/` is copied into a generated workspace when a
project resolves `kit.{{project_name}}`.

Paths there are relative to the workspace root, not to `tree/`. A file at
`tree/mk/kit_{{project_name}}.mk` lands at
`<workspace>/mk/kit_{{project_name}}.mk`.

**This file is not payload.** It sits beside `tree/`, inside the cartridge but
outside the part that ships into a workspace, so it documents the kit without
being installed by it.

That distinction is not academic. A `README.md` under `tree/` would land as
`<workspace>/README.md` and collide with the one the project template already
contributes — which is exactly what the first draft of this template did, and
what generating from it caught.

Name payload files after the kit for the same reason. `mk/kit_{{project_name}}.mk`
is unique; `mk/kit.mk` would collide with the next kit someone writes.

A kit that contributes build configuration puts a make fragment here; the
workspace Makefile includes `mk/kit_*.mk` by wildcard, so the fragment is
picked up with no edit to the workspace.

Note the ownership block in the manifest: `default` is `generated`, because
every file a kit contributes is the kit's to rewrite. That is the opposite of
a project template, where the author owns the tree. If some file should
survive a user's edits, name it in `user` explicitly.
