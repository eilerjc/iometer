# ICF Parser Unification Plan (Tier 3.6)

**Goal:** One ICF parser in `src/core`, used by both GUIs. **The MFC GUI's parsing
behavior is canonical** — every quirk, key spelling, version rule, and error path it
implements today must keep working byte-for-byte for backwards compatibility. Qt
converges onto it; MFC becomes thin adapters over the core parser.

---

## 1. Current state

### 1.1 MFC architecture (the behavior oracle)

Orchestrated by `GalileoView::OpenConfigFile(infilename, flags, replace)`. Each
subsystem **re-opens the file independently** and `SkipTo`'s its own section —
this is what enables *selective restore* (the Open dialog's checkboxes):

| Flag | Section | Parser | Notes |
|---|---|---|---|
| `ICFTestSetupFlag` | `'TEST SETUP` | `CPageSetup::LoadConfig` | GalileoView.cpp:1403 |
| `ICFResultsDisplayFlag` | `'RESULTS DISPLAY` | `CPageDisplay::LoadConfig` | :1451 |
| `ICFGlobalAspecFlag` | `'ACCESS SPECIFICATIONS` | `AccessSpecList::LoadConfig` | :1413, version-dispatched |
| `ICFManagerWorkerFlag` + `ICFAssignedAspecFlag` + `ICFAssignedTargetFlag` | `'MANAGER LIST` | `ManagerList::LoadConfig` → `Manager::LoadConfig` → `Worker::LoadConfig` | :1432; `load_aspecs`/`load_targets` thread through |

Primitives: `ICF_ifstream` (ICF_ifstream.h:92) — `GetVersion()`, `SkipTo(id)`,
`GetNextLine()`, `GetPair(key, value)`, static `ExtractFirstInt/UInt64/IntVersion/Token`.

### 1.2 Canonical behaviors that MUST be preserved (from code reading)

**Version handling** (`ICF_ifstream::GetVersion`, ICF_ifstream.cpp) — *corrected
by Phase-0 empirical testing*:
- Dual scheme: `major > 1900` ⇒ **date** version, `major*10000 + middle*100 +
  minor` (e.g. `1998.10.08` → `19981008`); else ⇒ **semver**,
  `major*10000000000 + middle*100000 + minor` (e.g. `1.1.0` → `10000100000`).
  Semver therefore always orders ABOVE every date version (intended: modern
  files always take the new-format paths).
- The `version == 32697` special case (`3.26.97` OLTP.txt → `19980105`) is
  **DEAD CODE under the current formula**: `3.26.97` computes to `30002600097`,
  never `32697`, so such a file dispatches to the NEW access-spec parser (and
  errors on old-format content). **Verified empirically in Phase 0**
  (oltp_32697 golden). Core must replicate this exactly — including keeping the
  unreachable special case unreachable.
- `< 19980105` ⇒ error message but **continues** (the `return -1` is commented out).
- Access-spec format switch: `version >= 19980521` ⇒ new format (`LoadConfigNew`),
  else old format (`LoadConfigOld`) **followed by `AssignDefaultAccessSpecs()` when
  replacing** (AccessSpecList.cpp:958-994).

**Replace vs merge**: `replace=TRUE` ⇒ `DeleteAll()` + `InsertIdleSpec()` before
loading specs; managers/workers similarly replaced; `replace=FALSE` merges.

**Missing sections are OK**: `SkipTo("'ACCESS SPECIFICATIONS")` failing returns
TRUE ("no access spec list to restore (this is OK)"). Same for other sections.

**Key spellings with multiple historical variants** (must all parse —
Worker.cpp:2186ff):
- `'Number of outstanding IOs,test connection rate,transactions per connection`
  *and* `...,use fixed seed,fixed seed value` (newer).
- `'Disk maximum size,starting sector` *and* `...,Data pattern` (newer).
- `'Default Workers to Spawn` (ancient) *and* `'Default Disk Workers to Spawn` /
  `'Default Network Workers to Spawn`, each accepting `NUMBER_OF_CPUS` tokens.
- `'Ramp Up Time` matched by **prefix** (`key.Left(strlen(...))`), not equality.
- `'Bar chart` matched by prefix, with sub-keys `statistic`, `manager ID, manager
  name`, `worker ID, worker name` (PageDisplay.cpp:1133ff).

**Case-insensitivity**: every key/token compare is `CompareNoCase`. Tokens like
`ENABLED/DISABLED`, `ALL/NO_TARGETS/NO_WORKERS/NO_MANAGERS/NONE`,
`LAST_UPDATE/WHOLE_TEST` likewise.

**Worker-level richness** (Worker.cpp:2186-2500): default target settings (queue
depth, test connection rate, trans/conn, fixed seed), disk max size / starting
sector / data pattern, `'Local network interface`, `'VI outstanding IOs`, target
assignments with per-target types — **with side-effects**: assigned spec names are
validated against the live `AccessSpecList`, targets matched against discovered
drives/NICs, defaults already assigned by the Worker constructor are removed when
the file says otherwise.

**Error reporting**: precise `ErrorMessage(...)` dialogs at the exact failure
point ("File is improperly formatted. ...") and parse abort semantics per section.

### 1.3 Core/Qt today (`core/IcfFile`)

Single-pass line parser producing `TestConfig` + `vector<AccessSpec>` +
`vector<BatchWorker{name,type,specs,targets,managerName/Address/Id}>`.

**Gaps vs MFC** (each is a work item):
1. No version handling at all (skips the `Version` line). No old-format specs.
2. No selective restore / replace-merge semantics (always whole-file).
3. RESULTS DISPLAY section skipped entirely (no display settings model).
4. TEST SETUP partial: missing default-workers-to-spawn (incl. `NUMBER_OF_CPUS`),
   record-results level tokens, suspend-test, cycling parameters only partially
   carried.
5. Worker detail: missing queue depth, conn-rate/trans-per-conn, fixed seed, disk
   size/start/data-pattern, network interface, VI fields, per-target types.
6. Exact-match keys only in places where MFC does prefix/case-insensitive match.
7. No error-message channel (just bool).

---

## 2. Target design

### 2.1 New core pieces (all pure C++, `iocore`, unit-testable)

**`core/IcfStream`** — faithful port of `ICF_ifstream` onto `std::ifstream` +
`std::string`:
`getVersion()` (returns the encoded `uint64_t`, with the 32697 special case and
the dual scheme), `skipTo()`, `getNextLine()`, `getPair()`,
`extractFirstInt/UInt64/IntVersion/Token` — same trimming/tab/comment semantics.
Plus `lastError()` carrying the exact MFC message text.

**`core/IcfDocument`** — section-oriented loaders mirroring the MFC orchestration
(NOT a single-pass parse; each loader re-scans from its section, exactly like MFC):
```
struct IcfLoadFlags { bool testSetup, resultsDisplay, globalSpecs,
                      managerWorker, assignedSpecs, assignedTargets; };
uint64_t version() const;
bool loadTestSetup(TestSetup &out);              // full PageSetup field set
bool loadResultsDisplay(DisplaySettings &out);   // NEW core model (update freq/type,
                                                 //   record-last-update, 6 bar charts)
bool loadAccessSpecs(std::vector<AccessSpec> &out);  // new AND old formats,
                                                 //   dispatched on version()
bool loadManagerList(std::vector<ManagerConfig> &out,
                     bool loadAspecs, bool loadTargets);
std::string lastError() const;
```
`ManagerConfig{name,id,address, workers:vector<WorkerConfig>}` and `WorkerConfig`
gains **every** MFC worker field (queue depth, connRate, transPerConn, fixedSeed
pair, diskMaxSize/startSector/dataPattern, networkInterface, viOutstandingIos,
targets with types). Existing `IcfFile::load` becomes a thin convenience over
`IcfDocument` (Qt API unchanged).

**Decisions that stay OUT of core** (front-end adapter responsibilities):
- Spec-name validation against the live spec list; target matching against
  discovered drives; Worker-constructor default-spec removal; the
  `AssignDefaultAccessSpecs()` call for old-format+replace; replace-vs-merge
  mutation of live lists; error **dialogs** (core supplies the text).

### 2.2 MFC adapters (phase by phase)

Each MFC `LoadConfig` keeps its signature and side-effects but delegates parsing:
e.g. `AccessSpecList::LoadConfigNew(infile)` body becomes: core
`loadAccessSpecs` → copy each `AccessSpec` into a `Test_Spec*` via the existing
friendly→wire mapping (`iocore::fillWireAccess` + name/default_assignment), then
the existing post-conditions. `ErrorMessage(core.lastError())` preserves dialogs.

---

## 3. Validation strategy (the gate — nothing ships without it)

1. **Golden corpus** (new `test/fixtures/icf_compat/`):
   - All current fixtures (already passing).
   - A **date-version** ICF (`Version 1998.10.08`), an **old-format access spec**
     ICF (< 1998.05.21), the **`3.26.97` OLTP** special case, a `< 19980105` file
     (warn-but-continue), files with each section missing, both key-spelling
     variants for worker settings and disk size, `NUMBER_OF_CPUS` spawn counts,
     prefix-matched `'Ramp Up Time (s)` variants, mixed-case keys.
2. **Behavioral goldens before any change (Phase 0)**: run the **current** MFC
   `IOmeter.exe /c <fixture>` (batch + dynamotest, non-elevated path from the
   smoke suite) over the corpus and store the result CSVs + a debug dump of the
   restored state. After each phase, re-run: **identical output required.**
3. **Round-trip**: MFC `SaveConfig` output must parse identically through core
   (save side unifies later in Tier 3.7, but parse-compat is asserted now).
4. **Unit tests** per loader (version table, each key variant, case folding,
   missing sections, malformed lines = MFC error text).
5. **Existing suites stay green at every phase**: Qt unit tests (22), smoke 5/5,
   MFC build clean.

---

## 4. Phased execution (each phase independently shippable)

| Phase | Content | Risk | Est. size |
|---|---|---|---|
| **0** | Compat corpus + golden harness (capture current MFC behavior; a `compare_icf_load.ps1` runner) | none (no product code) | S |
| **1** | `core/IcfStream` + version handling, unit tests (incl. 32697, dual scheme, warn-continue) | low | S-M |
| **2** | `loadTestSetup` + `loadResultsDisplay` + new core `TestSetup`/`DisplaySettings` models; Qt routed; MFC `CPageSetup`/`CPageDisplay` adapters | low-med (display model is new) | M |
| **3** | `loadAccessSpecs` new + **old** format; Qt routed; MFC `AccessSpecList::LoadConfigNew/Old` adapters (wire conversion via `fillWireAccess`; old-format `AssignDefaultAccessSpecs` stays in adapter) | med | M |
| **4** | `loadManagerList` full `WorkerConfig`; Qt `loadConfig` routed; MFC `ManagerList/Manager/Worker::LoadConfig` adapters. **Riskiest** — worker side-effects stay MFC-side; do `Manager` and `Worker` as separate sub-steps | high | L |
| **5** | Cleanup: delete dead MFC parse bodies; `ICF_ifstream` remains only for save side | low | S |
| **6** | (= Tier 3.7) Save-side unification through `core/IcfFile::save` — separate plan once 0-5 are green | high | L |

Rules of engagement per phase: MFC builds + smoke 5/5 + golden-corpus diff clean
**before merging the phase**; any behavioral difference is a core bug (MFC is the
oracle), never "fixed" by changing the fixture.

---

## 5. Known risks & mitigations

| Risk | Mitigation |
|---|---|
| `Worker::LoadConfig` side-effects (live spec/target validation) silently lost | Side-effects stay in MFC adapter; core returns pure data; Phase-4 sub-steps with goldens after each |
| Historical key variants missed | Corpus has one fixture per variant, built from the actual `CompareNoCase` strings in code (§1.2) |
| `CString` semantics (trim/tabs/`%` handling) differ from `std::string` port | `IcfStream` unit tests are written from `ICF_ifstream.cpp` behavior, not from intuition; fuzz the two against each other in a temporary MFC test harness if needed |
| Old-format spec parsing has no living test today | Build the old-format fixture by reading `LoadConfigOld` and verifying it loads in the **unmodified** MFC GUI first (Phase 0) |
| Error-text drift breaks user expectations | Core carries the exact MFC strings; adapters display verbatim |
| Same-basename .obj collisions in `Iometer.vcxproj` (seen with ManagerMap) | New core files get `<ObjectFileName>core_*.obj` entries |
| ALL_CAPS macro collisions in MFC context (seen with `HOSTNAME_LOCAL`) | No ALL_CAPS identifiers in new core headers |

---

## 6. Out of scope (tracked separately)

- Save-side unification (Tier 3.7 / Phase 6).
- `ManagerList` counts/assignment logic (Tier 8) and result decode (Tier 9).
- Qt UI for selective restore (Qt can pass all-flags-true until its Open dialog
  grows checkboxes; the core API supports it from day one).
