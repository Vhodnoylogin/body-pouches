# Native slot suspension: development integration

This unreleased change is based on the published Body Pouches0.2.0 source, with the current canonical synergy documents merged into the same development branch. It is a narrow consumer migration, not generic Body Slots presentation or an inventory/input transaction API. The original release archive is unchanged.

## Configuration and ownership

An exclusive pouch uses VRIK's suspended-slot path, which already admits the slot independently of weapon categories. Its configuration prerequisite is visibility; Body Pouches no longer enables `allowSmallSlotN` for that mode. Shared pouches retain native weapon-category configuration. The existing `mayEnableSlots` opt-in still controls changes to VRIK settings in memory, including visibility. Global holster/display settings, tracking, physical hand policy and current reach remain VRIK's runtime decisions; the configuration verdict does not establish those facts.

When VRIK advertises `VRIK_NativeSuspension_QueryInterface`, the adapter uses the new copied contract in `contract/vriknativesuspension_api.h`. Each configured exclusive native slot obtains this consumer's own claim with a caller-chosen2000ms lifetime. The HIGGS frame callback renews desired claims only after the existing working/load gate. Expiry or a load generation change requires a new claim handle. Clearing this consumer's claim cannot clear another consumer's claim. Clients remain process-local and the mod does not support dynamic unloading of its API providers.

Ordinary VRIK without that export keeps the existing001 suspension call. A service that advertises the new export but rejects negotiation, registration, copying or renewal does not trigger a fallback to the unowned001 boolean. Desired claims may be retried on later eligible frames; failure is not permission to take over native state. The existing getter reports effective suspension from all owners, not proof that Body Pouches owns a claim.

The claim suppresses VRIK's native weapon action. It does not grant exclusive input, content-provider rights, HIGGS possession, inventory stock or preview ownership. The old global holster callback and Body Pouches' existing item operations remain in use. Generic new-slot presentation, owner-scoped callbacks, committed transfers and automatic equipment identity remain separate work.

## Offline verification

The core configuration checks cover exclusive/shared modes, visibility and native numeric thresholds. The production VRIK adapter is also compiled against an explicit API/settings/loader boundary double; this checks its actual calls and never qualifies the real001 vtable. Failure to negotiate the advertised service must not write the old suspension boolean.

`bodypouches_suspension_consumer` compiles the actual new consumer class and loads only the separately built `NativeSuspensionFixture.dll`, containing VRIK's production numeric owner and ABI facade. It verifies independent owners, renewal, removal, real2000ms expiry, handle replacement and an explicit failing-renewal proxy. It refuses the filename `VRIK.dll`; the engine-dependent plugin and game are never loaded by this check. The provider repository documents its own core, ABI, native decision and lifecycle checks.

Build the offline targets with `BODYPOUCHES_BUILD_PLUGIN=OFF`, then run CTest from the generated `tests` directory. Run the separate consumer executable with the explicit isolated-provider DLL path. Full plugin builds and offline tests do not establish real anchors, callback cadence, loading, rendered output or potion transactions in Skyrim. A new game test is required before release.
