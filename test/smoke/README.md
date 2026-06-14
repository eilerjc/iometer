# Iometer smoke suite

One smoke suite, two thin wrappers, one shared definition.

- **`scenarios.json`** — the single, platform-neutral source of truth. It lists
  every scenario with a logical controller/worker name, args, an ICF, and a
  pass/fail rule. Both runners read this file; neither hard-codes the scenario
  list.
- **`run_smoke.ps1`** — Windows runner (PowerShell 5.1+). Covers every scenario,
  including the MFC ones.
- **`run_smoke.sh`** — Linux/macOS runner (Bash + `jq`). Covers the scenarios
  whose `platforms` include `linux` (everything except the Windows-only MFC GUI).

Each runner maps the logical binary names (`IometerQt`, `dynamotest`, `Dynamo`,
`Iometertest`, `IOmeter`) to real per-OS paths; nothing platform-specific lives
in the manifest.

## Modes

| Mode | Worker | Elevation | What it proves |
|------|--------|-----------|----------------|
| `test` (default) | `dynamotest` — synthetic disk, AsInvoker | none | Logic/plumbing end-to-end, deterministic, CI-friendly |
| `real` | production `Dynamo` — real disk | **admin/root** | True I/O against a physical drive |

`dynamotest` is the compile-time test build of Dynamo (`IOMTR_TEST_MODE`): it
services I/O synthetically and ships an AsInvoker manifest, so the whole suite
runs with **no UAC / no root**. The MFC GUI auto-spawns it (instead of the
RequireAdministrator `Dynamo`) when built with `IOMTR_SPAWN_DYNAMOTEST` — that is
what `Iometertest.exe` is.

## Usage

PowerShell (Windows):

```powershell
cd test\smoke
.\run_smoke.ps1                       # test mode (non-elevated)
.\run_smoke.ps1 -List                 # list scenarios, run nothing
.\run_smoke.ps1 -Only qt-dynamotest,unit-tests
.\run_smoke.ps1 -Mode real            # real Dynamo; self-elevates (UAC)
.\run_smoke.ps1 -Mode both
.\run_smoke.ps1 -IncludePending       # also run scenarios marked pending
```

Bash (Linux/macOS — needs `jq`, a built `IometerQt`, and a `dynamo`):

```bash
cd test/smoke
export IOMETER_QT=/path/to/IometerQt
export IOMETER_DYNAMO=/path/to/dynamo
export IOMETER_DYNAMOTEST=/path/to/dynamo-testmode   # optional; defaults to IOMETER_DYNAMO
./run_smoke.sh
./run_smoke.sh --list
./run_smoke.sh --only qt-dynamotest,unit-tests
sudo ./run_smoke.sh --mode real
```

Exit code is `0` only if every run scenario passed.

## Scenarios

| Scenario | Kind | Modes | Platforms | Notes |
|----------|------|-------|-----------|-------|
| `unit-tests` | ctest | test | win, linux | Qt + IometerCore unit suites |
| `icf-validate` | validate-icf | test | win, linux | Shipped app parses every fixture ICF |
| `qt-demo` | demo | test | win, linux | Demo engine starts/runs/exits |
| `qt-dynamotest` | batch-worker | test | win, linux | Qt batch + dynamotest worker |
| `mfc-dynamotest` | batch-selfspawn | test | win | **pending** (see below) |
| `qt-real` | batch-worker | real | win | **pending** (see below) |
| `mfc-real` | batch-selfspawn | real | win | Known-good reference (real disk) |

`batch-worker` = the runner starts the worker. `batch-selfspawn` = the GUI
auto-spawns its own worker (MFC). `pending` scenarios are skipped unless you pass
`-IncludePending` / `--include-pending`.

## Adding a scenario

Add an object to `scenarios.json` — no runner code needed for the existing
`kind`s. A new `kind` must be implemented in **both** runners to stay in sync.
`verify` supports `iopsGt` (IOps strictly greater than) and `errorsEq` (exact
error count); both are checked against the CSV `ALL` aggregate row.

## Known issues (tracked as `status: pending`)

- **`mfc-dynamotest`** — The `IOMTR_SPAWN_DYNAMOTEST` build makes `Iometertest.exe`
  auto-spawn `dynamotest` non-elevated; the spawn and TCP login succeed (port 1066
  opens, the worker connects), but the test **stalls during worker/target setup**
  before the first I/O cycle, so no results are produced. The same `dynamotest`
  drives the Qt GUI fine (`qt-dynamotest` passes), so the hang is in the MFC test
  driver's disk-target prepare path against the synthetic disk — to be fixed.
- **`qt-real`** — The Qt GUI against the **real** Dynamo on a real disk errors on
  every I/O (0 IOps / high error count), while `mfc-real` is healthy on the same
  ICF. The Qt engine itself is sound (`qt-dynamotest` passes cleanly), so this is
  specific to the real-disk path — to be fixed.
