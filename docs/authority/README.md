# AxLicense Authority

This directory is the target home for AxLicense human-readable Authority.

## Source-of-truth transition

AxLicense is migrating its Product & System Authority from Notion into this repository. During migration, source-of-truth is determined per artifact, not per workspace:

- an artifact still marked `pending` or `migrating` in `MIGRATION.md` remains Current Authority at its recorded Notion source;
- an artifact marked `cutover` has its Current Authority in this repository;
- repository code is Implementation Reality unless a document explicitly designates it as Authority;
- `.aegis/` stores machine-readable project-control state and provenance;
- `.aegis/state.json` is a reproducible projection/cache, not Authority.

Do not edit a migrated Authority in Notion after cutover. Preserve the Notion page as historical provenance and redirect readers to the repository copy.

## Canonical target layout

```text
docs/authority/
  discovery/
  modeling/
  architecture/
  verification/
  implementation/
  governance/
```

The migration proceeds in dependency order: P00, P01, P02, P03, P10, P11, P12, P13, P14, P15, P16 and its Flow Atlas, P17, P18, P20, P30, then implementation-control and governance records.

See `MIGRATION.md` for the bootstrap inventory and cutover ledger.
