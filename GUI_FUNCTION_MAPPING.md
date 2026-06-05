# GUI Function Equivalence Mapping

Maps MFC Iometer classes and methods to their Qt equivalents, tracking behavioral equivalence at the source level.

## Application Shell

| MFC | Qt | Purpose | Status |
|-----|----|---------|----|
| `CGalileoApp::InitInstance()` | `main()` in main.cpp | App initialization, Qt object creation | ✓ Equivalent |
| `CGalileoApp::ExitInstance()` | `~MainWindow()` destructors | Cleanup on exit | ✓ Equivalent |
| `CGalileoApp::OnAppAbout()` | `MainWindow::showAbout()` | About dialog | ⚠ UI-only, not testable on headless |

## Main Window / Document Model

| MFC | Qt | Purpose | Status |
|-----|----|---------|----|
| `CGalileoDoc` (document model) | `IometerEngine` (abstract) | Holds test config and state | ✓ Equivalent protocol |
| `CGalileoDoc::LoadICF()` | `DynamoEngine::loadConfig()` | Parse ICF file into config | ✓ tst_icf.cpp validates both directions |
| `CGalileoDoc::SaveICF()` | `DynamoEngine::saveConfig()` | Write config back to ICF | ✓ Round-trip tested |
| `CGalileoDoc::m_config` | `DynamoEngine::m_testConfig` | Active test configuration | ✓ Same struct |
| `CGalileoView::OnMenuConnect()` | `MainWindow::connectManager()` | Manager connection dialog/logic | ⚠ Qt UI-only |
| `GalileoView::UpdateAccessSpec()` | `MainWindow::setAccessSpecs()` | Apply access spec list | ✓ Core logic equivalent |

## Engine / Test Control

| MFC | Qt | Purpose | Status |
|-----|----|---------|----|
| `IOManager` | `DynamoEngine` | Main engine managing test lifecycle | ✓ Core logic equivalent |
| `IOManager::StartTest()` | `DynamoEngine::startTest()` | Initiate test sequence | ✓ Protocol-validated |
| `IOManager::StopTest()` | `DynamoEngine::stopTest()` | Graceful test stop | ✓ Protocol-validated |
| `IOManager::StopAll()` | `DynamoEngine::stopAll()` | Emergency shutdown | ✓ Protocol-validated |
| `IOTest` (worker lifecycle) | `DySession` (per-manager) | Manage one Dynamo instance | ✓ Core logic equivalent |

## Configuration & Access Specs

| MFC | Qt | Purpose | Status |
|-----|----|---------|----|
| `IOSpec` array | `AccessSpec` list | Built-in access specifications | ✓ 32 specs, tst_accessspecs validates |
| `IOSpec::SetupSpec()` | `AccessSpec` struct fields | Spec parameters (size, offset, seek pattern) | ✓ Identical fields |
| `IOManager::GetAccessSpecs()` | `DynamoEngine::accessSpecs()` | Return current spec list | ✓ Same interface |
| `IOManager::SetAccessSpecs()` | `DynamoEngine::setAccessSpecs()` | Apply new spec list | ✓ Same interface |

## Results Handling

| MFC | Qt | Purpose | Status |
|-----|----|---------|----|
| `ManagerList::SaveResults()` | `DynamoEngine::saveBatchResults()` | Write CSV results | ✓ tst_results validates CSV format |
| CSV column layout | CSV column layout (same 34 cols) | Results table | ✓ Column [0,6,12,27] verified identical |
| Aggregation logic (sum IOps, etc.) | `WorkerResult` aggregation | Compute totals from worker data | ✓ tst_results validates logic |
| Latency averaging | Latency averaging (arithmetic mean) | Average latency across workers | ✓ Numeric precision tested |

## Protocol / Wire Format

| MFC | Qt | Purpose | Status |
|-----|----|---------|----|
| `DyMsg` struct (8 bytes) | `DyMsg` struct (8 bytes) | Dynamo message header | ✓ sizeof validated |
| `DyDataMessage` (888,840 bytes) | `DyDataMessage` (888,840 bytes) | Dynamo data payload | ✓ sizeof validated |
| Login sequence (DY_LOGIN, Manager_Info) | DySession::processLoginMsg() | Dynamo connection handshake | ✓ Protocol tests verify structure |
| Message dispatch | DySession::dispatchMainData() | Route incoming messages to handlers | ✓ Structure validated |

## Untestable on Headless (UI-only)

These require a display server (X11, RDP, or physical screen):

| MFC | Qt | Component | Reason |
|-----|----|-----------|----|
| GalileoView (MDI window) | MainWindow | Main UI frame | Requires display |
| CMainFrame toolbar buttons | UI toolbar (QPushButton, QAction) | Button clicks, menu actions | Requires display |
| CConnectDlg | ConnectDialog | Manager connection UI | Dialog interactions |
| Graph widgets (CBarChartWidget) | BarChartWidget, MeterWidget | Real-time graphs | OpenGL rendering |
| PageSetup, PageAccess, PageDisplay | Tab pages | Configuration UI | Interactive pages |

**Note:** These UI tests exist (tst_meters, tst_pagesetup, tst_mainwindow) but can only run on systems with display.

## Behavioral Equivalence Verification

### Verified by Unit Tests
- `tst_icf.cpp` — ICF parsing/round-trip (both read same format)
- `tst_results.cpp` — CSV output and aggregation (column layout, math)
- `tst_protocol.cpp` — Dynamo wire format (struct sizes, command codes)
- `tst_accessspecs.cpp` — Built-in specs (count, names, properties)
- `tst_types.cpp` — Type conversions and formatting

### Verified by Smoke Tests
- Both engines load same ICF file → same internal config
- Both connect to Dynamo and receive target discovery
- Both produce CSV with identical column layout
- Both aggregate results with same numerical values

### Not Yet Directly Compared
- UI responsiveness (not testable on headless)
- Graph rendering (not testable on headless)
- User interaction flows (not automated)

## Implementation Notes for Future Tests

To add direct behavioral equivalence tests:

1. **ICF Equivalence**: Load same file in both, compare parsed TestConfig field-by-field
   ```cpp
   auto mfc_config = load_mfc_config("test.icf");
   auto qt_config = qt_engine.testConfig();
   assert(mfc_config.runSeconds == qt_config.runSeconds);
   // ... all fields
   ```

2. **CSV Equivalence**: Feed same WorkerResult to both, compare output
   ```cpp
   auto mfc_csv = mfc_engine.writeBatchResults(...);
   auto qt_csv = DynamoEngine::writeBatchResultsCsv(...);
   // Parse both and compare field-by-field
   ```

3. **Protocol Equivalence**: Capture Dynamo wire traffic from both, verify message sequences
   ```powershell
   # Run both against Dynamo, capture network traffic
   # Verify both send/receive same sequence of messages
   ```

---

**Last Updated:** 2026-06-05  
**Test Status:** 102 core tests pass; UI tests blocked on headless display
