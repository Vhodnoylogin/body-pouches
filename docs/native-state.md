# Read-only native state

Candidate0.1.11 adds the optional DevBench inspect extension
`body_pouches_state`. DevBench is not a gameplay dependency. Without a compatible
host (extension ABI build10500 or later), the mod simply registers no extension.
There is no mutation action, test mode, new server or direct draw/stow endpoint.

Requests use `inspect` with `kind: "body_pouches_state"`,
`action: "snapshot"` (default) or `"capabilities"`, and optional integer
`timeoutMs` from100 through3000 (default1000). Unknown fields/actions are refused.
Requests are limited to1024bytes and two pending main-thread tasks.

Snapshots copy actual state on the SKSE main thread under the mod's pouch lock.
JSON serialization and host response occur on the listener thread. A timeout,
cancellation, changed load epoch or failed load invalidates the read; expired
queued work does not inspect the next loaded world. Uninitialized dependencies,
loading, startup worlds, no current-generation callback, missing player3D/cell,
paused gameplay and transition menus produce unavailable state, not defaults.

The schemaVersion1 envelope identifies provider/version and the actual process:
`process.pid`, `process.createdFileTime100ns` and `process.identityAvailable`.
The birth value is Windows FILETIME in100ns units since1601UTC; preserve the full
64-bit integer. `sample.sequence`, `sample.producerMonotonicNs`,
`sample.lifecycleEpoch` and `sample.phase` describe the native read. The clock is
`std::chrono::steady_clock` in nanoseconds; only differences within a process
are meaningful. Samples from different process/load identities must not be joined.

`subject.available` and the envelope's `available`/`ok` gate the values below.
An unavailable subject has a reason and no settings/runtime values. Consumers
must propagate unavailable data, not translate absence into zero or false.

| Path | Meaning |
|---|---|
| `subject.settings.language` | Actual `Loc::Language()` selector; individual untranslated keys may still use built-in English |
| `subject.settings.logLevel` | Actual spdlog level after applying the mod's fallback rules |
| `subject.settings.mayEnableSlots` | Loaded native permission flag |
| `subject.settings.pouches` | Actual normalized core pouch map as slot/mode; duplicate configured slots appear once, unknown modes have become shared |
| `subject.settings.inputHandedness` | Current native game `bLeftHandedMode:VRInput` as left/right; null if unavailable |
| `subject.inputHandednessAvailable` | Whether the native game setting existed |
| `subject.runtime.assignmentCount` | Actual number of assigned core pouches, in assigned_pouches; independent of inventory quantity |
| `subject.bodySlots[]` | Actual `VRIK GetSlotSuspended(slot)` per loaded pouch, with slot, suspended and suspendedAvailable; never inferred from exclusive/shared mode |
| `subject.loadGeneration` | Mod's actual world-invalidation counter; initial world may be zero; both pre/post-load invalidation can increment it |
| `subject.callbackFrame` | Mod's actual HIGGS post-VRIK/post-HIGGS callback counter; **not** an engine, physics or render-frame number |

The mod caches handedness when its ordinary hand-translation helpers first run.
The diagnostic reads the game setting without initializing/changing those caches.
It therefore establishes the actual game setting, not that a later in-process
setting change has updated cached translations. Handedness fixtures must start
a new process and verify actual physical-hand behavior separately.

One snapshot is a bounded read at `skse_main_thread_task`, not exhaustive event
continuity or an earliest-frame display trace. It does not create runtime potions,
move controllers, restart the process, apply settings or restore a fixture.
Those are separate qualified platform responsibilities. No live-game qualification
has been claimed for this candidate. Installed0.1.10 and immutable order19 remain
unchanged; using this new binary requires a new installed build and order pins.

The MIT DevBench API glue in `contract/` is copied unchanged from the published
integration files at upstream commit32823e465c18a676afc9048ac948aa22a616abd0.
Its license is shipped by the ordinary staging document list. The DevBench
plugin implementation is not included.
