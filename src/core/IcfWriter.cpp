#include "IcfWriter.h"
#include <iomanip>

namespace iocore {

// The section header rules are copied verbatim (as adjacent string literals) from
// the MFC SaveConfig methods so the "====" run width is byte-identical.

void IcfWriter::writeVersionLine(std::ostream &os, const std::string &version)
{
    os << "Version " << version << "\n";
}

// ---- 'TEST SETUP (port of CPageSetup::SaveConfig) -------------------------------

void IcfWriter::writeTestSetup(std::ostream &os, const IcfTestSetup &ts)
{
    os << "'TEST SETUP ============================" "========================================" << "\n";

    os << "'Test Description" << "\n" << "\t" << ts.testName << "\n";

    os << std::left;   // MFC: setiosflags(ios::left)

    os << "'Run Time" << "\n"
       << "'\t" << std::setw(11) << "hours" << std::setw(11) << "minutes" << "seconds" << "\n"
       << "\t" << std::setw(11) << ts.hours << std::setw(11) << ts.minutes << ts.seconds << "\n";

    os << "'Ramp Up Time (s)" << "\n" << "\t" << ts.rampTime << "\n";

    os << "'Default Disk Workers to Spawn" << "\n" << "\t";
    if (ts.diskWorkerCount == -1) os << "NUMBER_OF_CPUS" << "\n";
    else                          os << ts.diskWorkerCount << "\n";

    os << "'Default Network Workers to Spawn" << "\n" << "\t";
    if (ts.netWorkerCount == -1)  os << "NUMBER_OF_CPUS" << "\n";
    else                          os << ts.netWorkerCount << "\n";

    os << "'Record Results" << "\n" << "\t";
    switch (ts.resultType) {
    case 1:  os << "NO_TARGETS"  << "\n"; break;
    case 2:  os << "NO_WORKERS"  << "\n"; break;
    case 3:  os << "NO_MANAGERS" << "\n"; break;
    case 4:  os << "NONE"        << "\n"; break;
    default: os << "ALL"         << "\n"; break;   // 0 = RecordAll
    }

    os << "'Worker Cycling" << "\n"
       << "'\t" << std::setw(11) << "start" << std::setw(11) << "step" << "step type" << "\n"
       << "\t" << std::setw(11) << ts.workerCycling.start << std::setw(11) << ts.workerCycling.step
       << (ts.workerCycling.stepType == 1 ? "EXPONENTIAL" : "LINEAR") << "\n";

    os << "'Disk Cycling" << "\n"
       << "'\t" << std::setw(11) << "start" << std::setw(11) << "step" << "step type" << "\n"
       << "\t" << std::setw(11) << ts.targetCycling.start << std::setw(11) << ts.targetCycling.step
       << (ts.targetCycling.stepType == 1 ? "EXPONENTIAL" : "LINEAR") << "\n";

    os << "'Queue Depth Cycling" << "\n"
       << "'\t" << std::setw(11) << "start" << std::setw(11) << "end" << std::setw(11) << "step" << "step type" << "\n"
       << "\t" << std::setw(11) << ts.queueCycling.start << std::setw(11) << ts.queueCycling.end
       << std::setw(11) << ts.queueCycling.step
       << (ts.queueCycling.stepType == 1 ? "EXPONENTIAL" : "LINEAR") << "\n";

    os << "'Test Type" << "\n" << "\t";
    switch (ts.testType) {
    case 1:  os << "CYCLE_TARGETS"                     << "\n"; break;
    case 2:  os << "CYCLE_WORKERS"                     << "\n"; break;
    case 3:  os << "INCREMENT_TARGETS_PARALLEL"        << "\n"; break;
    case 4:  os << "INCREMENT_TARGETS_SERIAL"          << "\n"; break;
    case 5:  os << "CYCLE_WORKERS_AND_TARGETS"         << "\n"; break;
    case 6:  os << "CYCLE_OUTSTANDING_IOS"             << "\n"; break;
    case 7:  os << "CYCLE_OUTSTANDING_IOS_AND_TARGETS" << "\n"; break;
    default: os << "NORMAL"                            << "\n"; break;   // 0 = NORMAL
    }

    os << "'END test setup" << "\n";
    os << std::right;   // leave the stream as we found it
}

// ---- 'RESULTS DISPLAY (port of CPageDisplay::SaveConfig) -------------------------

void IcfWriter::writeResultsDisplay(std::ostream &os, const IcfDisplaySettings &ds)
{
    os << "'RESULTS DISPLAY =======================" "========================================" << "\n";

    os << "'Record Last Update Results,Update Frequency,Update Type" << "\n" << "\t";
    os << (ds.recordLastUpdate ? "ENABLED" : "DISABLED");
    os << "," << ds.updateFrequency;
    os << "," << (ds.whichPerf == 1 ? "LAST_UPDATE" : "WHOLE_TEST") << "\n";

    // Each loaded bar entry maps to one output line; entries for the same bar
    // (statistic, then manager, then worker) are consecutive, reproducing the
    // grouped MFC layout.
    for (const auto &bar : ds.bars) {
        const int n = bar.index + 1;   // file is one-based
        switch (bar.kind) {
        case IcfDisplaySettings::Bar::Statistic:
            os << "'Bar chart " << n << " statistic" << "\n" << "\t" << bar.name << "\n";
            break;
        case IcfDisplaySettings::Bar::Manager:
            os << "'Bar chart " << n << " manager ID, manager name" << "\n"
               << "\t" << bar.id << "," << bar.name << "\n";
            break;
        case IcfDisplaySettings::Bar::Worker:
            os << "'Bar chart " << n << " worker ID, worker name" << "\n"
               << "\t" << bar.id << "," << bar.name << "\n";
            break;
        }
    }

    os << "'END results display" << "\n";
}

// ---- 'ACCESS SPECIFICATIONS (port of AccessSpecList::SaveConfig) -----------------

void IcfWriter::writeAccessSpecs(std::ostream &os, const std::vector<IcfAccessSpec> &specs)
{
    os << "'ACCESS SPECIFICATIONS =================" "========================================" << "\n";

    for (const auto &spec : specs) {
        os << "'Access specification name,default assignment" << "\n";
        os << "\t" << spec.name << ",";
        switch (spec.defaultAssignment) {
        case 1:  os << "ALL"  << "\n"; break;
        case 2:  os << "DISK" << "\n"; break;
        case 3:  os << "NET"  << "\n"; break;
        default: os << "NONE" << "\n"; break;   // 0 = AssignNone
        }

        os << "'size,% of size,% reads,% random,delay,burst,align,reply" << "\n";
        for (const auto &l : spec.lines) {
            os << "\t" << l.size << "," << l.ofSize << "," << l.reads << "," << l.random
               << "," << l.delay << "," << l.burst << "," << l.align << "," << l.reply << "\n";
        }
    }

    os << "'END access specifications" << "\n";
}

// ---- 'MANAGER LIST (port of ManagerList/Manager/Worker::SaveConfig) --------------

void IcfWriter::writeWorker(std::ostream &os, const IcfWorkerConfig &w,
                            bool saveAspecs, bool saveTargets)
{
    os << "'Worker" << "\n" << "\t" << w.name << "\n" << "'Worker type" << "\n";
    switch (w.kind) {
    case IcfWorkerKind::NetTCP: os << "\tNETWORK" << ",TCP" << "\n"; break;
    case IcfWorkerKind::NetVI:  os << "\tNETWORK" << ",VI"  << "\n"; break;
    default:                    os << "\tDISK"               << "\n"; break;
    }

    os << "'Default target settings for worker" << "\n";
    os << "'Number of outstanding IOs,test connection rate,transactions per connection,use fixed seed,fixed seed value" << "\n";
    os << "\t" << w.queueDepth
       << "," << (w.testConnRate ? "ENABLED" : "DISABLED")
       << "," << w.transPerConn
       << "," << (w.useFixedSeed ? "ENABLED" : "DISABLED")
       << "," << w.fixedSeedValue << "\n";

    if (w.kind == IcfWorkerKind::Disk) {
        os << "'Disk maximum size,starting sector,Data pattern" << "\n";
        os << "\t" << w.diskMaxSize << "," << w.startingSector << "," << w.dataPattern << "\n";
    }
    if (w.kind == IcfWorkerKind::NetTCP || w.kind == IcfWorkerKind::NetVI) {
        os << "'Local network interface" << "\n" << "\t" << w.localNetworkInterface << "\n";
    }
    if (w.kind == IcfWorkerKind::NetVI) {
        os << "'VI outstanding IOs" << "\n" << "\t" << w.viOutstandingIos << "\n";
    }
    os << "'End default target settings for worker" << "\n";

    if (saveAspecs) {
        os << "'Assigned access specs" << "\n";
        for (const auto &s : w.assignedSpecNames)
            os << "\t" << s << "\n";
        os << "'End assigned access specs" << "\n";
    }

    if (saveTargets) {
        os << "'Target assignments" << "\n";
        for (const auto &t : w.targets) {
            os << "'Target" << "\n";
            switch (t.kind) {
            case IcfWorkerKind::NetTCP:
                os << "\t" << t.name << "\n" << "'Target type" << "\n" << "\tNETWORK,TCP" << "\n";
                os << "'Target manager ID, manager name" << "\n"
                   << "\t" << t.targetManagerId << "," << t.targetManagerName << "\n";
                break;
            case IcfWorkerKind::NetVI:
                os << "\t" << t.name << "\n" << "'Target type" << "\n" << "\tNETWORK,VI" << "\n";
                os << "'Target manager ID, manager name" << "\n"
                   << "\t" << t.targetManagerId << "," << t.targetManagerName << "\n";
                break;
            default:   // Disk
                os << "\t" << t.name << "\n" << "'Target type" << "\n" << "\tDISK" << "\n";
                break;
            }
            os << "'End target" << "\n";
        }
        os << "'End target assignments" << "\n";
    }

    os << "'End worker" << "\n";
}

void IcfWriter::writeManagerList(std::ostream &os,
                                 const std::vector<IcfManagerConfig> &managers,
                                 bool saveAspecs, bool saveTargets)
{
    os << "'MANAGER LIST ==========================" "========================================" << "\n";

    for (const auto &m : managers) {
        os << "'Manager ID, manager name" << "\n" << "\t" << m.id << "," << m.name << "\n";
        os << "'Manager network address" << "\n" << "\t" << m.address << "\n";
        for (const auto &w : m.workers)
            writeWorker(os, w, saveAspecs, saveTargets);
        os << "'End manager" << "\n";
    }

    os << "'END manager list" << "\n";
}

} // namespace iocore
