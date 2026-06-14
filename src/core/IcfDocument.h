#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Section loaders for ICF configuration files - faithful ports of the MFC
// section parsers (the canonical behavior; see docs/dev/ICF_PARSER_UNIFICATION_PLAN.md).
// Phase 2 covers 'TEST SETUP (CPageSetup::LoadConfig) and 'RESULTS DISPLAY
// (CPageDisplay::LoadConfig). Each loader opens its own stream and seeks its
// own section, exactly like the MFC code, so loaders are independent and a
// front-end can restore sections selectively.
//
// Loader semantics (shared):
//  - missing section        -> true (nothing to restore; "this is OK")
//  - bad version line       -> false
//  - parse errors           -> false, with the EXACT MFC dialog texts in
//                              errors() (in dialog order; some "Ignoring..."
//                              errors record text and continue, like MFC)
//  - loaders OVERWRITE ONLY the fields whose keys are present in the file
//    (MFC writes directly into page members, retaining prior values for
//    missing keys) - callers pre-fill structs with current state.

namespace iocore {

// 'TEST SETUP - numeric values mirror the MFC enums (raw integers can flow in
// from legacy files, so the encodings must match):
//   resultType:  RecordAll=0, NoTargets=1, NoWorkers=2, NoManagers=3, None=4
//   stepType:    StepLinear=0, StepExponential=1
//   testType:    NORMAL=0, CyclingTargets=1, CyclingWorkers=2,
//                IncTargetsParallel=3, IncTargetsSerial=4, WorkersTargets=5,
//                CyclingQueue=6, CyclingQueueTargets=7
//   workerCount: -1 = NUMBER_OF_CPUS
struct IcfTestSetup {
    std::string testName;
    int hours = 0, minutes = 0, seconds = 0;
    int rampTime = 0;
    int diskWorkerCount = 0;
    int netWorkerCount  = 0;
    int resultType = 0;
    struct Cycling { int start = 1, end = 1, step = 1, stepType = 0; };
    Cycling workerCycling, targetCycling, queueCycling;   // queue uses end too
    int testType = 0;
};

// 'RESULTS DISPLAY - whichPerf: WHOLE_TEST_PERF=0, LAST_UPDATE_PERF=1.
struct IcfDisplaySettings {
    struct Bar {
        enum Kind { Statistic, Manager, Worker };
        int         index = 0;     // zero-based (file is one-based, 1..6)
        Kind        kind  = Statistic;
        std::string name;          // statistic name / manager name / worker name
        int         id    = 0;     // manager/worker ID (unused for Statistic)
    };
    bool updateLinePresent = false;   // the Record/Frequency/Type key was seen
    bool recordLastUpdate  = false;
    int  updateFrequency   = 0;
    int  whichPerf         = 0;
    std::vector<Bar> bars;            // in file order; apply sequentially
};

// 'ACCESS SPECIFICATIONS - raw wire-format numbers, exactly as in the file.
// defaultAssignment mirrors the MFC enum: AssignNone=0, AssignAll=1,
// AssignDisk=2, AssignNet=3 (raw integers can flow in from legacy files).
struct IcfAccessSpecLine {
    int size = 0, ofSize = 0, reads = 0, random = 0;
    int delay = 0, burst = 0, align = 0, reply = 0;
};
struct IcfAccessSpec {
    std::string name;
    int defaultAssignment = 0;
    std::vector<IcfAccessSpecLine> lines;
};

// 'MANAGER LIST - the per-manager / per-worker configuration, as raw parsed data.
// The MFC live-object side-effects (loadmap manager lookup, access-spec name
// validation, target matching against discovered disks / remote-manager NICs)
// are the front-end adapter's job; the loader returns pure data only.
// Invalid mirrors MFC's InvalidType: it is what a worker keeps when its type
// token is an unknown NETWORK subtype (MFC records the error but does NOT abort,
// leaving wkr_type_hex unchanged - InvalidType for the first such worker).
enum class IcfWorkerKind { Invalid = -1, Disk = 0, NetTCP = 1, NetVI = 2 };
struct IcfTargetConfig {
    std::string    name;
    IcfWorkerKind  kind = IcfWorkerKind::Disk;   // DISK / NETWORK,TCP / NETWORK,VI
    // Network targets only: the remote manager that owns the target NIC.
    int            targetManagerId = 0;
    std::string    targetManagerName;
};
struct IcfWorkerConfig {
    std::string    name;
    IcfWorkerKind  kind = IcfWorkerKind::Disk;
    // 'Default target settings for worker (all optional; MFC defaults shown):
    int            queueDepth     = 1;
    bool           testConnRate   = false;
    int            transPerConn   = 1;
    bool           useFixedSeed   = false;
    uint64_t       fixedSeedValue = 0;
    uint64_t       diskMaxSize    = 0;
    uint64_t       startingSector = 0;
    int            dataPattern    = 0;
    std::string    localNetworkInterface;
    int            viOutstandingIos = 0;
    bool           hasDefaultSettings = false;   // a settings block was present
    // Which 'Default target settings lines actually appeared. MFC writes a
    // setting ONLY when its key is present; an absent key keeps the worker's
    // prior value, so a faithful adapter must apply selectively.
    bool           ioSettingsPresent    = false; // 'Number of outstanding IOs... line
    bool           fixedSeedPresent     = false; // ...the use-fixed-seed variant
    bool           diskSizePresent      = false; // 'Disk maximum size,starting sector line
    bool           dataPatternPresent   = false; // ...the Data pattern variant
    bool           localNetIfacePresent = false; // 'Local network interface
    bool           viIosPresent         = false; // 'VI outstanding IOs
    std::vector<std::string>     assignedSpecNames;
    std::vector<IcfTargetConfig> targets;
};
struct IcfManagerConfig {
    std::string                  name;
    int                          id = 1;
    std::string                  address;
    std::vector<IcfWorkerConfig> workers;
};

class IcfDocument {
public:
    explicit IcfDocument(const std::string &path) : m_path(path) {}

    bool loadTestSetup(IcfTestSetup &io);
    bool loadResultsDisplay(IcfDisplaySettings &io);

    // Load the 'MANAGER LIST section (managers, their workers, each worker's
    // default settings, assigned spec NAMES and target NAMES+types). A faithful
    // port of ManagerList/Manager/Worker::LoadConfig's PARSING; the caller does
    // the live matching/validation. loadAspecs/loadTargets false skips (but still
    // consumes) those worker sub-sections, like the MFC flags. Network targets
    // record their owning manager (targetManagerId/Name); the live NIC matching
    // and CreateNetClient remain the front-end adapter's job.
    bool loadManagerList(std::vector<IcfManagerConfig> &out,
                         bool loadAspecs = true, bool loadTargets = true);

    // Load the global access specifications, dispatching on the file version
    // like AccessSpecList::LoadConfig: >= 1998.05.21 uses the modern named
    // format; older files use the raw numeric format (specs are then named by
    // the SmartName rules and default-assigned to all workers). New-format
    // specs REPLACE an earlier spec of the same name (case-insensitive) within
    // `out`. `existingNames` (optional) are names already in the caller's live
    // list - they participate in the old format's Untitled/SmartName
    // uniqueness checks, like the MFC live-list lookups did.
    bool loadAccessSpecs(std::vector<IcfAccessSpec> &out,
                         const std::vector<std::string> *existingNames = nullptr);

    // The file's encoded version (IcfStream::BadVersion on error). Does not
    // touch errors(); used by adapters that need format decisions (e.g. the
    // old-format default-spec assignment).
    uint64_t readVersion() const;

    // MFC dialog texts from the LAST load call, in the order MFC would have
    // shown them (front-end adapters display these verbatim).
    const std::vector<std::string> &errors() const { return m_errors; }

private:
    std::string              m_path;
    std::vector<std::string> m_errors;
};

} // namespace iocore
