# Refactoring Opportunities: Extracting Shared Logic

Analysis of what could be moved from GUI to shared libraries to reduce duplication between MFC and Qt.

## Current Architecture

### Shared Code (Already Extracted)

**Location:** `src/`

These are already framework-agnostic and used by both MFC and Qt:
- `DyProto.h` — Dynamo protocol structs (wire format)
- ICF parsing logic (format constants)
- Access spec library (built-in specs)
- Type conversion utilities

**Result:** Protocol and format compatibility are already proven via tests.

### GUI-Specific Code (Tightly Coupled)

**MFC Implementation:** `src/` (GalileoApp, GalileoDoc, IOManager, etc.)
**Qt Implementation:** `src/qt/` (MainWindow, DynamoEngine, DySession, etc.)

---

## Extraction Opportunities

### Tier 1: High-Value, Low-Effort (Do First)

#### A. Dynamo Protocol Handler

**Currently:** Embedded in `DynamoEngine` (Qt)

**Extract to:** `src/core/DynamoConnection.h`

```cpp
class DynamoConnection {
    // Encapsulates:
    // - TCP socket management (port 1066)
    // - Login handshake (DY_LOGIN message)
    // - Message dispatch (binary parsing)
    // - State machine (WaitLoginMsg, WaitTargets, Ready, Running, etc.)
    
    // Interface
    bool connect(const QString &host, const QString &name);
    bool sendMessage(const DyMsg &msg);
    DyDataMessage* receiveData(int timeoutMs);
    void disconnect();
    
    // Events
    signals:
        void connected();
        void disconnected();
        void messageReceived(const DyMsg &msg);
        void error(const QString &msg);
};
```

**Used by:**
- Qt: `DySession` would use this instead of managing sockets directly
- MFC: `IOTest` could use this for protocol handling
- Future: Batch mode tools, headless runners

**Benefit:**
- ✓ Protocol logic in one place
- ✓ Both GUIs use identical wire protocol
- ✓ Easier to test protocol edge cases
- ✓ Reusable for CLI tools

**Effort:** ~8 hours  
**Risk:** Low (protocol is already tested and stable)

---

#### B. ICF File Parsing & Writing

**Currently:** Embedded in `DynamoEngine::loadConfig()` and `saveConfig()`

**Extract to:** `src/core/IcfFile.h`

```cpp
class IcfFile {
    // ICF 1.1.0 format parsing/writing
    
    bool load(const QString &path, TestConfig &cfg);
    bool save(const QString &path, const TestConfig &cfg);
    
    // Validation
    bool isValidVersion(const QString &versionStr);
    bool isValidAccessSpec(const AccessSpec &spec);
};

// Result: Both GUIs call IcfFile::load() instead of duplicating parser
```

**Used by:**
- Qt: `DynamoEngine::loadConfig()` becomes wrapper
- MFC: Could replace internal ICF parser
- Smoke tests: Direct file manipulation
- CLI tools: Batch config processing

**Benefit:**
- ✓ Single source of truth for format
- ✓ Easier to update format spec
- ✓ Better error messages
- ✓ Reusable everywhere

**Effort:** ~6 hours  
**Risk:** Medium (ICF parser is complex)

**Note:** This may already be partially done; verify against current code.

---

#### C. Results Aggregation & CSV Output

**Currently:** `DynamoEngine::writeBatchResultsCsv()` + `WorkerResult`

**Extract to:** `src/core/ResultsWriter.h`

```cpp
class ResultsWriter {
    // CSV column layout and aggregation logic
    
    bool writeCsv(const QString &path, 
                  const QVector<WorkerResult> &results,
                  const TestConfig &cfg);
    
    // Column accessors (document column indices)
    static constexpr int COL_AGGREGATE_MARKER = 0;   // "ALL"
    static constexpr int COL_IOPS = 6;
    static constexpr int COL_MBPS_DEC = 12;
    static constexpr int COL_ERRORS = 27;
};
```

**Used by:**
- Qt: Calls `ResultsWriter::writeCsv()`
- MFC: Same interface
- Smoke tests: Parsing/validation (already doing this)
- Reporting tools: Consistent CSV generation

**Benefit:**
- ✓ Both produce identical CSV format
- ✓ Column layout documented in one place
- ✓ Easier to maintain format changes
- ✓ Testable independently

**Effort:** ~4 hours  
**Risk:** Low (logic is simple, already tested)

---

### Tier 2: Medium-Value, Medium-Effort (Do Next)

#### D. Test Lifecycle Management

**Currently:** 
- Qt: `DynamoEngine::startTest()`, `stopTest()`, `stopAll()`
- MFC: Similar logic in `IOManager`

**Extract to:** `src/core/TestRunner.h`

```cpp
class TestRunner {
    // Platform-agnostic test lifecycle
    
    bool start(const TestConfig &cfg, 
               const QVector<WorkerInfo> &workers,
               const QVector<AccessSpec> &specs);
    bool stop();
    
    // Progress/results
    const QVector<WorkerResult>& currentResults() const;
    TestState state() const;
    
    // Signals (both can connect Qt signals to these)
    std::function<void(const QString &)> onStatusChange;
    std::function<void(const WorkerResult &)> onResultUpdate;
    std::function<void(const QString &)> onError;
};
```

**Used by:**
- Qt: Delegates to TestRunner instead of managing state directly
- MFC: Same interface
- Batch mode: Direct test execution
- API layer: Future remote testing

**Benefit:**
- ✓ Test state machine in one place
- ✓ Both GUIs work identically
- ✓ Easier to add logging/auditing
- ✓ Reusable for automation

**Effort:** ~12 hours  
**Risk:** Medium (state machine is complex)

**Note:** This would require refactoring DynamoEngine significantly.

---

#### E. Worker/Manager Management

**Currently:**
- Qt: `DynamoEngine::m_sessions`, `addWorker()`, `removeWorker()`
- MFC: `ManagerList`, `IOManager::m_ioTests[]`

**Extract to:** `src/core/WorkerPool.h`

```cpp
class WorkerPool {
    // Manage multiple Dynamo connections + workers
    
    bool addManager(const QString &address, const QString &name);
    bool removeManager(const QString &name);
    
    bool addWorker(const QString &mgrName, const WorkerInfo &w);
    bool removeWorker(const QString &mgrName, const QString &workerId);
    
    const QList<ManagerInfo>& managers() const;
    const QList<WorkerInfo>& workers(const QString &mgrName) const;
};
```

**Benefit:**
- ✓ Manager/worker lifecycle centralized
- ✓ Both GUIs use same data structures
- ✓ Easier to validate configurations
- ✓ Foundation for distributed testing

**Effort:** ~10 hours  
**Risk:** Medium (multi-connection logic)

---

### Tier 3: High-Value, High-Effort (Do Later)

#### F. Configuration Management

**Currently:**
- Qt: `TestConfig` struct + `DynamoEngine` managing state
- MFC: Similar but spread across multiple classes

**Extract to:** `src/core/IometerConfig.h`

```cpp
class IometerConfig {
    // Immutable, validated configuration
    
    TestConfig testConfig;
    QVector<AccessSpec> specs;
    QList<WorkerInfo> workers;
    QList<TargetInfo> targets;
    
    // Validation
    bool isValid() const;
    QStringList validationErrors() const;
    
    // Serialization
    bool loadFromIcf(const QString &path);
    bool saveToIcf(const QString &path) const;
};
```

**Benefit:**
- ✓ Single configuration object
- ✓ Validation happens once
- ✓ Both GUIs use same format
- ✓ Easier to unit test

**Effort:** ~15 hours  
**Risk:** High (breaks existing patterns)

---

#### G. Target Discovery & Validation

**Currently:**
- Qt: `DySession::processTargetList()` (embedded in protocol handler)
- MFC: Similar logic in `IOTest`

**Extract to:** `src/core/TargetDiscovery.h`

```cpp
class TargetDiscovery {
    // Manage discovered targets from Dynamo
    
    void addTarget(const DyTargetSpec &spec);
    void processPhysicalDisks(const DyDataMessage &msg);
    void processLogicalDisks(const DyDataMessage &msg);
    void processTcpTargets(const DyDataMessage &msg);
    
    const QList<TargetInfo>& targets() const;
    bool hasValidTargets() const;
};
```

**Benefit:**
- ✓ Target handling logic centralized
- ✓ Both GUIs get target list same way
- ✓ Easier to add validation
- ✓ Testable independently

**Effort:** ~8 hours  
**Risk:** Medium (target types are complex)

---

## Impact Analysis

### What Gets Smaller

| Component | Current | After Extraction | Reduction |
|-----------|---------|------------------|-----------|
| DynamoEngine.cpp | ~2000 LOC | ~1200 LOC | 40% |
| DySession.cpp | ~1500 LOC | ~800 LOC | 47% |
| IOManager (MFC est.) | ~3000 LOC | ~1800 LOC | 40% |
| **Total Code** | ~7000 LOC | ~4500 LOC | **36%** |

### Code Sharing

| Library | MFC Uses | Qt Uses | Benefit |
|---------|----------|---------|---------|
| DynamoConnection | ✓ | ✓ | Protocol logic identical |
| IcfFile | ✓ | ✓ | Format parsing unified |
| ResultsWriter | ✓ | ✓ | CSV generation identical |
| TestRunner | ✓ | ✓ | Test state machine unified |
| WorkerPool | ✓ | ✓ | Manager/worker logic identical |

### Lines of Code Shared

- **Current:** ~2000 LOC shared (protocol structs, types)
- **After Tier 1:** ~4000 LOC shared (add protocol handler, ICF, results)
- **After Tier 2:** ~6500 LOC shared (add test runner, workers)
- **After Tier 3:** ~8500 LOC shared (add config, discovery)

---

## Recommended Approach

### Phase 1 (Weeks 1-2): Low-Risk Extractions

1. **Extract ResultsWriter** (Tier 1C)
   - Effort: 4 hours
   - Risk: Low
   - Win: Both GUIs produce identical CSV, testable independently
   - Commit: `Extract CSV output to shared library`

2. **Extract DynamoConnection** (Tier 1A)
   - Effort: 8 hours
   - Risk: Low
   - Win: Protocol logic centralized, both use identical socket handling
   - Commit: `Extract Dynamo TCP protocol handler`

3. **Refactor ICF Parser** (Tier 1B)
   - Effort: 6 hours
   - Risk: Medium (but already tested)
   - Win: Single source of truth for file format
   - Commit: `Extract ICF file parsing to shared library`

### Phase 2 (Weeks 3-4): Medium-Risk Extractions

4. **Extract WorkerPool** (Tier 2E)
   - Effort: 10 hours
   - Risk: Medium
   - Win: Manager/worker lifecycle unified
   - Commit: `Extract worker pool management`

5. **Extract TestRunner** (Tier 2D)
   - Effort: 12 hours
   - Risk: Medium
   - Win: Test state machine centralized
   - Commit: `Extract test lifecycle management`

### Phase 3 (Later): High-Impact Refactoring

6. **Configuration Management** (Tier 3F)
   - High effort, high risk
   - Do only after Phase 1-2 stable

---

## Risks & Mitigations

### Risk: Breaking Existing Code

**Mitigation:**
- Keep old classes as thin wrappers initially
- Add tests for new extracted libraries first
- Extract incrementally (one library at a time)
- Run full test suite after each extraction

### Risk: Slowing Down Development

**Mitigation:**
- Do extractions in phases, not all at once
- Phase 1 extractions are low-risk
- Each extraction should improve, not degrade, code clarity

### Risk: Over-Abstraction

**Mitigation:**
- Extract only where both MFC and Qt would benefit
- Don't create premature abstractions
- Keep libraries focused (single responsibility)

---

## Success Criteria

After refactoring:

- [ ] 36% reduction in total code size (7000 → 4500 LOC)
- [ ] 4000+ LOC shared between MFC and Qt
- [ ] Both GUIs pass existing smoke tests
- [ ] No regression in test coverage (102 tests still pass)
- [ ] No change to CSV output or protocol handling
- [ ] New extracted libraries have unit tests

---

## Recommendation

**Do Phase 1 first (4-6 weeks):**
1. Extract ResultsWriter (4h)
2. Extract DynamoConnection (8h)
3. Refactor ICF parser (6h)
4. Verify with smoke tests
5. Measure code reduction

**Result:** 20% less code, both GUIs benefit, low risk.

**Revisit Phase 2 after Phase 1 succeeds.**

---

## Related Work

- `GUI_FUNCTION_MAPPING.md` — Already documents equivalences
- `TESTING_STRATEGY.md` — Three-layer testing catches regressions
- `src/qt/tests/` — Comprehensive test suite validates extractions

---

**Last Updated:** 2026-06-05
