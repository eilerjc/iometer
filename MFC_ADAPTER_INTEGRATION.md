# MFC Adapter Integration Plan

## Overview
Integrate IOManagerAdapter into Manager class to use pure C++ core libraries alongside existing MFC worker threads.

## Current State
- IOManagerAdapter.h/cpp created and implemented
- Manager class unchanged (ready for integration)
- Both systems can coexist during transition

## Integration Strategy: Parallel Operation

Instead of replacing Manager's worker management, run both in parallel:
1. IOManagerAdapter maintains state in core libraries
2. Manager continues operating grunts and handling protocol
3. Results from both systems can be compared/validated

## Phase 1: Add Adapter to Manager

```cpp
// In IOManager.h
class Manager {
    // ... existing members ...
    
private:
    IOManagerAdapter m_adapter;  // Add this
};
```

## Phase 2: Sync on Key Operations

### On DY_ADD_WORKERS
```cpp
void Manager::Add_Workers(int count) {
    // Existing grunt creation
    // ...
    
    // NEW: Add to adapter's WorkerPool
    ManagerInfo mgr;
    mgr.name = manager_name;
    m_adapter.getWorkerPool()->addManager(mgr);
    
    for (int i = 0; i < count; ++i) {
        WorkerInfo w;
        w.name = grunts[i]->GetWorkerName();
        w.id = std::to_string(i);
        m_adapter.getWorkerPool()->addWorker(mgr.name, w);
    }
}
```

### On DY_SET_TARGETS
```cpp
BOOL Manager::Set_Targets(int worker_no, int count, Target_Spec * target_specs) {
    // Existing target setting
    if (!grunts[worker_no]->Set_Targets(count, target_specs))
        return FALSE;
    
    // NEW: Update adapter
    WorkerInfo w;
    w.name = grunts[worker_no]->GetWorkerName();
    w.id = std::to_string(worker_no);
    for (int i = 0; i < count; ++i) {
        w.targets.push_back(IOManagerAdapter::mfcStringToStd(target_specs[i].name));
    }
    m_adapter.getWorkerPool()->updateWorker(w);
    
    return TRUE;
}
```

### On DY_SET_ACCESS
```cpp
BOOL Manager::Set_Access(int target, const Test_Spec * spec) {
    // Existing access spec setting
    if (!grunts[target]->Set_Access(spec))
        return FALSE;
    
    // NEW: Update adapter with access spec
    AccessSpec as = IOManagerAdapter::testSpecToAccessSpec(spec);
    // Store in adapter for test coordination
    
    return TRUE;
}
```

### On DY_START_TEST
```cpp
void Manager::Start_Test(int target) {
    // Existing test start
    // ...
    
    // NEW: Start test in adapter to synchronize state
    std::vector<WorkerInfo> workers = m_adapter.getWorkerPool()->workers(
        m_adapter.getWorkerPool()->managerInfos()[0].name);
    // ... build test config from existing m_testConfig ...
    m_adapter.getTestRunner()->startTest(cfg, workers, specs);
}
```

## Phase 3: Results Synchronization

After tests complete, convert Manager results to WorkerResult:
```cpp
void Manager::Report_Results(int which_perf) {
    // Existing reporting
    // ...
    
    // NEW: Convert to adapter format
    std::vector<WorkerResult> results;
    for (int i = 0; i < grunt_count; ++i) {
        WorkerResult wr = convertGruntResults(grunts[i]);
        results.push_back(wr);
    }
    m_adapter.m_results = results;
}
```

## Phase 4: Output Generation

Use shared ResultsWriter:
```cpp
// In Iometer (GUI), when saving results:
IOManagerAdapter adapter;
// ... populate with results ...
adapter.writeResults("results.csv");
// Uses same CSV format as Qt
```

## Benefits

1. **Minimal Initial Changes**: Manager continues working as-is
2. **Validation**: Can compare Manager results with adapter's TestRunner state
3. **Gradual Migration**: Can refactor piece by piece
4. **Code Sharing**: Both paths eventually use same ResultsWriter, IcfFile
5. **No Qt Dependency**: Adapter pure C++, no Qt needed for MFC build

## Helper Methods Needed in IOManagerAdapter

```cpp
// Additional conversions to add:
static WorkerResult convertGruntResults(Grunt *grunt);
static AccessSpec testSpecToAccessSpec(const Test_Spec *spec);  // Already exists
static WorkerInfo targetSpecToWorkerInfo(const Target_Spec *spec);
```

## Testing Approach

1. Add IOManagerAdapter member to Manager
2. Enable sync points one at a time
3. After each operation, verify adapter state matches grunt state
4. Run existing Manager tests - should all still pass
5. Later: Run integrated tests where both systems report same results

## Timeline

- **Week 1**: Add adapter member, hook Add_Workers
- **Week 2**: Sync Set_Targets and Set_Access
- **Week 3**: Sync Start_Test and Result reporting
- **Week 4**: Integration testing and validation
