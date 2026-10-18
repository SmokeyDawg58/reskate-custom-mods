# ReSkate 2.1.0 custom Throwdowns — local main merge

Local main now includes `reskate-mode-210` (`00c5fa2`), with the current Deathrace and enrollment fixes preserved. **This merge has not been pushed or published.** All 91 optimized checks pass; live testing of this exact runtime remains pending.

R.E.S.K.A.T.E. adds spectator input control, copied/failed/invalid-trick feedback, stable landing checks, combinations, spread return positions and diagrams that follow the game's Goofy stance setting. The custom-map GUID guard and external-pack HUD residency fix are included on the current 2.1.0 engine. Session protocol remains 52.

The feature merge keeps the pre-merge Deathrace editor, recovery, gate visibility, HUD, physical assets, host Start and enrollment logic. [Preservation audit](docs/DEATHRACE_PRESERVATION_20261018.json) and [detailed notes](docs/RELEASE_NOTES_20261018_RESKATE_210_LOCAL.md) record that boundary and the earlier Deathrace work carried forward.

Use the prepared local **ReSkate-2.1.0-RESKATE-210-Full.zip** and complete **Assets 1.0.7**, with the same runtime and assets on every PC. The output folder also supplies Runtime Update, standalone Assets, exact source overlay and current asset-authoring source. Close the game/launcher yourself before using the backup installer; run the checker afterward. No application is started or stopped by this merge.

There is no public download for this local merge. The [previous public enrollment playtest](https://github.com/SmokeyDawg58/reskate-custom-mods/releases/tag/v2.1.0-custom-20261018-enrollment-playtest) remains historical and has different runtime/asset hashes.

See [PLAYTEST.md](PLAYTEST.md), [DIAGNOSTICS.md](DIAGNOSTICS.md), [build guide](docs/BUILD.md), [asset guide](docs/ASSET_PACK.md), `mods.patch`, `SNAPSHOT.json` and `BUILD_SOURCES.json`. The source overlay reconstructs official ReSkate 2.1.0 at `ba1245a5af51892e90f2be8637dd69a7e20f5b77` exactly to the checked integration tree. No game executable, Steam DLL, personal settings, captures or credentials are distributed.

Maintained by SmokeyDawg58. R.E.S.K.A.T.E./map-fix contributions by hunterjedward123, with the branch's Claude Opus 5.5 AI co-author credit retained. Integration and checks received substantial OpenAI Codex AI assistance. Upstream attribution and GPL-3.0 are retained; see [CONTRIBUTORS.md](CONTRIBUTORS.md).
