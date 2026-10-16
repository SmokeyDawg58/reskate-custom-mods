# Multiplayer playtest diagnostics

79/79 automated checks pass. Installed runtime and all17 native payload files verified; settings and other mods preserved.

Human-confirmed two-PC checks: native guest flag/Join; local controls after finish/DNF; guest Quit without ending host race. Native HUD appearance and functional menu behavior remain the working baseline.

The guest flag currently inherits some Spot Battle wording and its waiting action can say Start. Only the host can start the custom race. Correct Deathrace wording and a guest Ready counter are being worked on separately; they are not included in this tested snapshot.

This is a controlled playtest release. The three human checks do not certify every map, player count or disconnect/reconnect scenario. No new load simulation was run for this snapshot; earlier production-server fake-transport simulations are historical evidence only.
