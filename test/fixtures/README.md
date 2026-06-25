# Test ICF fixtures

Shared `.icf` configs used by the smoke suite (`../smoke/`), the ICF goldens
(`icf_compat/`), and the GUI tests (`../../gui_tests/`).

## ⚠️ Empty-manager-address fixtures auto-spawn an *elevated* Dynamo under the MFC GUI

A fixture whose **single manager has a blank network address** is treated by the
**MFC `IOmeter.exe`** (when loaded with `/c`) as "this machine": it takes the
single-local-manager path in `ManagerList::LoadConfigPreprocess`, calls
`SpawnLocalManagers`, and launches the **RequireAdministrator `Dynamo.exe`** — i.e.
a **UAC elevation prompt**. (`IsAddressLocal("")` is true; an address of
`localhost` is *not* local — see `GalileoApp::IsAddressLocal` — so a localhost
manager makes the GUI *wait* for an external worker instead of spawning one.)
`IometerQt` never spawns, so loading the same file there is harmless.

Affected fixtures (single manager, blank address):

- `../smoke_test.icf`
- `cov_rich_disk.icf`
- `multispec.icf`
- `multi_target.icf`
- `multispec_worker.icf`
- `default_assign.icf`

**If you add an MFC (`IOmeter.exe`) test, do not point `/c` at these** unless you
actually want the elevation (e.g. the `mfc-real` real-disk scenario, which is
admin-only by nature). For a non-elevated MFC test use a **localhost-manager**
fixture — `nonlocal_manager.icf`, or anything under `icf_compat/` (all `localhost`)
— and start the worker yourself: `dynamotest -n <manager> -m localhost`.

> The warning can't live inside the `.icf` files: an extra comment line between
> the `Version` header and `'TEST SETUP` makes `IometerQt --validate-icf` reject
> the file (modal error → hang), so it lives here instead.
