#include "IcfFile.h"
#include "IcfDocument.h"
#include "IcfWriter.h"
#include <fstream>

const std::string IcfFile::VERSION_STRING = "1.1.0";

// --- Parsing helpers ---------------------------------------------------------

// Extract quoted string from manager list field (e.g., C: "System")
static std::string extractTarget(const std::string &s) {
    size_t start = s.find('"');
    size_t end = s.rfind('"');
    if (start != std::string::npos && end != std::string::npos && start < end) {
        return s.substr(start + 1, end - start - 1);
    }
    return s;
}

bool IcfFile::load(const std::string &filepath,
                   TestConfig &cfg,
                   std::vector<AccessSpec> &specs,
                   std::vector<BatchWorker> &workers)
{
    {
        std::ifstream probe(filepath);
        if (!probe) return false;   // missing/unreadable file
    }

    cfg = TestConfig();
    specs.clear();
    workers.clear();

    // Each section is parsed independently by the shared MFC-faithful loader
    // (iocore::IcfDocument), which opens its own stream and seeks its own
    // section - so no manual line scanning is needed here.

    // 'TEST SETUP - parsed by the shared MFC-faithful section loader
    // (iocore::IcfDocument). Pre-fill so keys absent from the file retain the
    // current values, exactly like the MFC page members.
    {
        iocore::IcfDocument doc(filepath);
        iocore::IcfTestSetup ts;
        ts.testName   = cfg.description;
        ts.hours      = cfg.runHours;
        ts.minutes    = cfg.runMinutes;
        ts.seconds    = cfg.runSeconds;
        ts.rampTime   = cfg.rampSeconds;
        ts.resultType = cfg.recordResults;
        ts.testType   = cfg.cyclingMode;
        ts.workerCycling.start    = cfg.workerStart;
        ts.workerCycling.step     = cfg.workerStep;
        ts.workerCycling.stepType = cfg.workerStepping;
        ts.targetCycling.start    = cfg.targetStart;
        ts.targetCycling.step     = cfg.targetStep;
        ts.targetCycling.stepType = cfg.targetStepping;
        ts.queueCycling.start     = cfg.ioqStart;
        ts.queueCycling.end       = cfg.ioqEnd;
        ts.queueCycling.step      = cfg.ioqPower;
        ts.queueCycling.stepType  = cfg.ioqStepping;

        if (!doc.loadTestSetup(ts))
            return false;   // malformed TEST SETUP - same as the MFC GUI

        cfg.description   = ts.testName;
        cfg.runHours      = ts.hours;
        cfg.runMinutes    = ts.minutes;
        cfg.runSeconds    = ts.seconds;
        cfg.rampSeconds   = ts.rampTime;
        // TestConfig's recordResults is a 0=All / 1=other model.
        cfg.recordResults = (ts.resultType == 0) ? 0 : 1;
        cfg.cyclingMode   = ts.testType;
        cfg.workerStart    = ts.workerCycling.start;
        cfg.workerStep     = ts.workerCycling.step;
        cfg.workerStepping = ts.workerCycling.stepType;
        cfg.targetStart    = ts.targetCycling.start;
        cfg.targetStep     = ts.targetCycling.step;
        cfg.targetStepping = ts.targetCycling.stepType;
        cfg.ioqStart       = ts.queueCycling.start;
        cfg.ioqEnd         = ts.queueCycling.end;
        cfg.ioqPower       = ts.queueCycling.step;
        cfg.ioqStepping    = ts.queueCycling.stepType;
    }

    // 'ACCESS SPECIFICATIONS - parsed by the shared MFC-faithful loader
    // (handles both the modern named format and the pre-1998.05.21 numeric
    // format, with the MFC validation rules).
    {
        iocore::IcfDocument doc(filepath);
        std::vector<iocore::IcfAccessSpec> rawSpecs;
        if (!doc.loadAccessSpecs(rawSpecs))
            return false;   // malformed spec section - same as the MFC GUI

        for (const auto &rs : rawSpecs) {
            AccessSpec spec;
            spec.name        = rs.name;
            // Friendly model keeps a single flag: any default assignment counts.
            spec.defaultSpec = (rs.defaultAssignment != 0);
            for (const auto &rl : rs.lines) {
                AccessSpecLine asl;
                asl.sizeBytes   = rl.size;
                asl.ofSize      = rl.ofSize;
                asl.readPercent = rl.reads;
                asl.seqPercent  = 100 - rl.random;
                asl.delayMs     = rl.delay;
                asl.burstLength = rl.burst;
                asl.alignBytes  = rl.align;
                asl.replyBytes  = rl.reply;
                spec.lines.push_back(asl);
            }
            if (!spec.lines.empty())
                specs.push_back(spec);
        }
    }

    // 'MANAGER LIST - parsed by the shared MFC-faithful loader, then flattened to
    // the front-end's BatchWorker list (one entry per worker, tagged with its
    // manager's name/address/id for ManagerMap-driven restore). The loader
    // enforces the exact MFC accept/reject rules; the (line-based) lenient scan
    // this replaces accepted some non-MFC inputs.
    {
        iocore::IcfDocument doc(filepath);
        std::vector<iocore::IcfManagerConfig> mgrs;
        if (!doc.loadManagerList(mgrs))
            return false;   // malformed MANAGER LIST - same as the MFC GUI

        for (const auto &mgr : mgrs) {
            for (const auto &w : mgr.workers) {
                if (w.name.empty())
                    continue;
                BatchWorker bw;
                bw.name           = w.name;
                bw.managerName    = mgr.name;
                bw.managerAddress = mgr.address;
                bw.managerId      = mgr.id;
                switch (w.kind) {
                    case iocore::IcfWorkerKind::NetTCP: bw.type = "TCP";  break;
                    case iocore::IcfWorkerKind::NetVI:  bw.type = "VI";   break;
                    default:                            bw.type = "DISK"; break;
                }
                bw.assignedSpecs = w.assignedSpecNames;
                for (const auto &t : w.targets)
                    bw.targets.push_back(extractTarget(t.name));
                workers.push_back(bw);
            }
        }
    }

    return true;
}

bool IcfFile::save(const std::string &filepath,
                   const TestConfig &cfg,
                   const std::vector<AccessSpec> &specs,
                   const std::vector<BatchWorker> &workers)
{
    std::ofstream out(filepath);
    if (!out) return false;

    // The on-disk format is produced by the shared, MFC-faithful section writers
    // (iocore::IcfWriter). This adapter just maps the front-end's friendly model
    // (TestConfig / AccessSpec / BatchWorker) into the canonical section structs.
    iocore::IcfWriter::writeVersionLine(out, VERSION_STRING);

    // 'TEST SETUP
    iocore::IcfTestSetup ts;
    ts.testName        = cfg.description;
    ts.hours           = cfg.runHours;
    ts.minutes         = cfg.runMinutes;
    ts.seconds         = cfg.runSeconds;
    ts.rampTime        = cfg.rampSeconds;
    ts.diskWorkerCount = 1;   // historical Qt batch defaults (1 disk / 0 net)
    ts.netWorkerCount  = 0;
    ts.resultType      = cfg.recordResults;   // 0 = ALL, else NO_TARGETS
    ts.workerCycling   = { cfg.workerStart, 1, cfg.workerStep, cfg.workerStepping };
    ts.targetCycling   = { cfg.targetStart, 1, cfg.targetStep, cfg.targetStepping };
    ts.queueCycling    = { cfg.ioqStart, cfg.ioqEnd, cfg.ioqPower, cfg.ioqStepping };
    ts.testType        = cfg.cyclingMode;
    iocore::IcfWriter::writeTestSetup(out, ts);

    // 'RESULTS DISPLAY - this friendly model doesn't track display bars, so write
    // the canonical default six-statistic layout (as the Qt save always has).
    iocore::IcfDisplaySettings ds;
    ds.recordLastUpdate = false;
    ds.updateFrequency  = 1;
    ds.whichPerf        = 1;   // LAST_UPDATE
    static const char *const kBarNames[6] = {
        "Total I/Os per Second", "Total MBs per Second (Decimal)",
        "Average I/O Response Time (ms)", "Maximum I/O Response Time (ms)",
        "% CPU Utilization (total)", "Total Error Count"
    };
    for (int i = 0; i < 6; ++i) {
        iocore::IcfDisplaySettings::Bar bar;
        bar.index = i;
        bar.kind  = iocore::IcfDisplaySettings::Bar::Statistic;
        bar.name  = kBarNames[i];
        ds.bars.push_back(bar);
    }
    iocore::IcfWriter::writeResultsDisplay(out, ds);

    // 'ACCESS SPECIFICATIONS
    std::vector<iocore::IcfAccessSpec> rawSpecs;
    for (const auto &spec : specs) {
        iocore::IcfAccessSpec rs;
        rs.name              = spec.name;
        rs.defaultAssignment = spec.defaultSpec ? 1 : 0;   // ALL / NONE
        for (const auto &l : spec.lines) {
            iocore::IcfAccessSpecLine rl;
            rl.size   = l.sizeBytes;
            rl.ofSize = l.ofSize;
            rl.reads  = l.readPercent;
            rl.random = 100 - l.seqPercent;
            rl.delay  = static_cast<int>(l.delayMs);
            rl.burst  = l.burstLength;
            rl.align  = l.alignBytes;
            rl.reply  = l.replyBytes;
            rs.lines.push_back(rl);
        }
        rawSpecs.push_back(rs);
    }
    iocore::IcfWriter::writeAccessSpecs(out, rawSpecs);

    // 'MANAGER LIST - the friendly model keeps a flat worker list; emit them under
    // one manager (TESTHOST), as the Qt save always has.
    std::vector<iocore::IcfManagerConfig> mgrs;
    if (!workers.empty()) {
        iocore::IcfManagerConfig mgr;
        mgr.id      = 1;
        mgr.name    = "TESTHOST";
        mgr.address = "";
        for (const auto &bw : workers) {
            // bw.type may be "DISK"/"TCP"/"VI" or already "NETWORK,TCP" etc.
            const bool isVi  = bw.type.find("VI") != std::string::npos;
            const bool isNet = isVi || bw.type.find("TCP") != std::string::npos
                                    || bw.type.find("NETWORK") != std::string::npos
                                    || bw.type.find("Network") != std::string::npos;
            iocore::IcfWorkerConfig w;
            w.name = bw.name;
            w.kind = !isNet ? iocore::IcfWorkerKind::Disk
                            : (isVi ? iocore::IcfWorkerKind::NetVI : iocore::IcfWorkerKind::NetTCP);
            // historical Qt defaults (the friendly model carries no per-worker settings)
            w.queueDepth = 1;
            w.transPerConn = 1;
            w.assignedSpecNames = bw.assignedSpecs;
            for (const auto &tname : bw.targets) {
                iocore::IcfTargetConfig t;
                t.name = tname;
                t.kind = w.kind;   // target type mirrors the worker
                if (isNet) { t.targetManagerId = mgr.id; t.targetManagerName = mgr.name; }
                w.targets.push_back(t);
            }
            mgr.workers.push_back(w);
        }
        mgrs.push_back(mgr);
    }
    iocore::IcfWriter::writeManagerList(out, mgrs, /*saveAspecs*/true, /*saveTargets*/true);

    iocore::IcfWriter::writeVersionLine(out, VERSION_STRING);
    out.close();
    return true;
}

bool IcfFile::isValidVersion(const std::string &versionStr)
{
    return versionStr.find("1.1.0") != std::string::npos;
}
