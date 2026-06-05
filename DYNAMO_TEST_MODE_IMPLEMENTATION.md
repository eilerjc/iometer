# Dynamo Test Mode Implementation

## Status

Created foundation files for test mode:
- `src/TestDataGenerator.h` - Interface for synthetic test data
- `src/TestDataGenerator.cpp` - Implementation of synthetic responses
- `src/msvs11/Dynamo.vcxproj` - Updated to include new files

## What's Done

✅ Created TestDataGenerator class with:
- getManagerInfo() - Synthetic Manager_Info
- getTargets() - Synthetic disk targets (C: and D:)
- getWorkerResults() - Synthetic I/O results with deterministic variation

✅ Updated Dynamo.vcxproj to compile new files

## What Remains (Phase 2 Option)

To fully integrate test mode (-test flag support) requires modifying:

1. **Pulsar.cpp** (main/ParseParam)
   - Add `-test` flag parsing to ParseParam() function
   - Set global `g_test_mode` flag

2. **IOPort.cpp** / **IOTarget.cpp** / **IOManager.cpp**
   - Add conditionals: `if (g_test_mode) { use TestDataGenerator; }`
   - Minimal 50-70 LOC total scattered across files

## Alternative: Use Qt Test Mode Instead

Since we've just completed Qt refactoring with ResultsWriter, DynamoConnection, and IcfFile extractions, the **Qt test mode is already more practical**:

✅ Qt already has test infrastructure
✅ No elevation needed (DemoEngine mode)
✅ No legacy code modifications
✅ Easier to iterate

## Recommendation

**For immediate CI/CD testing:**
1. Use Qt DemoEngine mode (already works, no elevation)
2. Keep MFC real-access smoke tests (when elevation available)
3. Implement Dynamo test mode as Phase 2 enhancement (if needed)

**If Dynamo test mode is desired:**
- Requires ~100-150 LOC in Pulsar/IOManager/IOPort files
- Would unify both engines under same test infrastructure
- Worth doing if test suite grows significantly

## Files Ready for Integration

Test data generator is compiled into Dynamo (via vcxproj update).
Ready to integrate into:
- IOManager::Initialize() 
- IOPort::ReceiveData()
- IOTarget::Enumerate()

Just needs the flag parsing and conditional checks.

---

**Verdict**: Foundation laid. Full integration optional based on testing needs.
