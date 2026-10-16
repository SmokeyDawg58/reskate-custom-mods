# Playtest diagnostics

79/79 automated checks pass. The new regression drives the actual race referee from Playing directly to Results and verifies the finish cue appears once. Recovery requests now survive a busy/off-board physics handoff and are acknowledged after the actual velocity write. Native rendering/audio and speed under live physics still require the checks in PLAYTEST.md.

The owner previously confirmed guest native flag Join, restored controls after finish/DNF and guest-only Quit on two PCs. Missed-gate recovery speed and the separate progress layout were confirmed for the preceding installed update. These confirmations do not certify every map, player count or disconnect case.

Asset mod 1.0.2 has 17 native files plus two exact quiet WAVs. Independent project comparison found only the shared R.E.S.K.A.T.E. logo and flag texture chunks changed among 1,635 resources. Native identities, metadata, bundles, HUD animations and all other payloads stayed exact. Every required file is hash-pinned to the matching lean runtime.

No new live load simulation was run for this artwork/recovery update. No logging, profiling, crash capture, PDBs, personal settings, game executable or Steam DLL is distributed. The native Spot Battle wording on one join confirmation remains a known issue, awaiting the owner's deferred live capture.
