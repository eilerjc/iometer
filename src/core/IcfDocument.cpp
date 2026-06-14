#include "IcfDocument.h"
#include "IcfStream.h"
#include "AccessSpecCatalog.h"   // smartNameText (shared with the live SmartName)
#include <cctype>
#include <cstdlib>

namespace iocore {

// ---- CString-semantics helpers (match the MFC parse code) ----------------------

static bool equalsNoCase(const std::string &a, const char *b)
{
    std::size_t i = 0;
    for (; b[i]; ++i) {
        if (i >= a.size())
            return false;
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return i == a.size();
}

static bool startsWithNoCase(const std::string &s, const char *prefix)
{
    for (std::size_t i = 0; prefix[i]; ++i) {
        if (i >= s.size())
            return false;
        if (std::tolower(static_cast<unsigned char>(s[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

static void trimBoth(std::string &s)
{
    const std::size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) { s.clear(); return; }
    const std::size_t e = s.find_last_not_of(" \t\r\n");
    s = s.substr(b, e - b + 1);
}

static void trimRight(std::string &s)
{
    const std::size_t e = s.find_last_not_of(" \t\r\n");
    s = (e == std::string::npos) ? std::string() : s.substr(0, e + 1);
}

// Parse a cycling line: ints (2 or 3 of them) then LINEAR/EXPONENTIAL/legacy int.
// Returns false (caller emits its own message) when an int is missing.
static bool parseCyclingTail(std::string &value, IcfTestSetup::Cycling &c, bool hasEnd)
{
    if (!IcfStream::extractFirstInt(value, c.start))
        return false;
    if (hasEnd && !IcfStream::extractFirstInt(value, c.end))
        return false;
    if (!IcfStream::extractFirstInt(value, c.step))
        return false;

    const std::string token = IcfStream::extractFirstToken(value);
    if (equalsNoCase(token, "LINEAR"))
        c.stepType = 0;                          // StepLinear
    else if (equalsNoCase(token, "EXPONENTIAL"))
        c.stepType = 1;                          // StepExponential
    else                                         // value used to be an integer
        c.stepType = std::atoi(token.c_str());
    return true;
}

// ---- 'TEST SETUP (port of CPageSetup::LoadConfig) -------------------------------

bool IcfDocument::loadTestSetup(IcfTestSetup &io)
{
    m_errors.clear();
    IcfStream infile(m_path);

    const uint64_t version = infile.getVersion();
    if (version == IcfStream::BadVersion) {
        m_errors = infile.errors();
        return false;
    }
    if (!infile.skipTo("'TEST SETUP"))
        return true;   // no test setup to restore (this is OK)

    std::string key, value, token;
    while (true) {
        if (!infile.getPair(key, value)) {
            m_errors.push_back("File is improperly formatted.  Expected "
                               "test setup data or \"END test setup\".");
            return false;
        }

        if (equalsNoCase(key, "'END test setup")) {
            break;
        } else if (equalsNoCase(key, "'Test Description")) {
            io.testName = value;
        } else if (equalsNoCase(key, "'Run Time")) {
            if (!IcfStream::extractFirstInt(value, io.hours)
                || !IcfStream::extractFirstInt(value, io.minutes)
                || !IcfStream::extractFirstInt(value, io.seconds)) {
                m_errors.push_back("Error while reading file.  "
                    "\"Run time\" should be specified as three integer values "
                    "(hours, minutes, seconds).");
                return false;
            }
        } else if (startsWithNoCase(key, "'Ramp Up Time")) {
            if (!IcfStream::extractFirstInt(value, io.rampTime)) {
                m_errors.push_back("Error while reading file.  "
                    "\"Ramp up time\" should be specified as an integer value.");
                return false;
            }
        }
        // For backward compatibility...
        else if (equalsNoCase(key, "'Default Workers to Spawn")) {
            if (!IcfStream::extractFirstInt(value, io.diskWorkerCount)) {
                m_errors.push_back("Error while reading file.  "
                    "\"Default workers to spawn\" should be "
                    "specified as an integer value.");
                return false;
            }
            if (io.diskWorkerCount == 0)   // "# of CPUs" used to be 0,
                io.diskWorkerCount = -1;   // now it is -1.
        } else if (equalsNoCase(key, "'Default Disk Workers to Spawn")) {
            token = IcfStream::extractFirstToken(value);
            if (equalsNoCase(token, "NUMBER_OF_CPUS"))
                io.diskWorkerCount = -1;
            else
                io.diskWorkerCount = std::atoi(token.c_str());
        } else if (equalsNoCase(key, "'Default Network Workers to Spawn")) {
            token = IcfStream::extractFirstToken(value);
            if (equalsNoCase(token, "NUMBER_OF_CPUS"))
                io.netWorkerCount = -1;
            else
                io.netWorkerCount = std::atoi(token.c_str());
        } else if (equalsNoCase(key, "'Record Results")) {
            trimBoth(value);
            if (equalsNoCase(value, "ALL"))              io.resultType = 0;
            else if (equalsNoCase(value, "NO_TARGETS"))  io.resultType = 1;
            else if (equalsNoCase(value, "NO_WORKERS"))  io.resultType = 2;
            else if (equalsNoCase(value, "NO_MANAGERS")) io.resultType = 3;
            else if (equalsNoCase(value, "NONE"))        io.resultType = 4;
            // This value used to be stored as an integer.
            else if (!IcfStream::extractFirstInt(value, io.resultType)) {
                m_errors.push_back("Error while reading file.  "
                    "For \"Record results\", expected a legal identifier or an "
                    "integer value.  See the documentation for details.");
                return false;
            }
        } else if (equalsNoCase(key, "'Worker Cycling")) {
            if (!parseCyclingTail(value, io.workerCycling, false)) {
                m_errors.push_back("Error while reading file.  "
                    "\"Worker cycling\" start and step should be specified as "
                    "integer values.");
                return false;
            }
        } else if (equalsNoCase(key, "'Disk Cycling")) {
            if (!parseCyclingTail(value, io.targetCycling, false)) {
                m_errors.push_back("Error while reading file.  "
                    "\"Disk cycling\" start and step should be specified as "
                    "integer values.");
                return false;
            }
        } else if (equalsNoCase(key, "'Queue Depth Cycling")) {
            if (!parseCyclingTail(value, io.queueCycling, true)) {
                m_errors.push_back("Error while reading file.  "
                    "\"Queue depth cycling\" start, end, and step should be "
                    "specified as integer values.");
                return false;
            }
        } else if (equalsNoCase(key, "'Test Type")) {
            trimBoth(value);
            if (equalsNoCase(value, "NORMAL"))                                io.testType = 0;
            else if (equalsNoCase(value, "CYCLE_TARGETS"))                    io.testType = 1;
            else if (equalsNoCase(value, "CYCLE_WORKERS"))                    io.testType = 2;
            else if (equalsNoCase(value, "INCREMENT_TARGETS_PARALLEL"))       io.testType = 3;
            else if (equalsNoCase(value, "INCREMENT_TARGETS_SERIAL"))         io.testType = 4;
            else if (equalsNoCase(value, "CYCLE_WORKERS_AND_TARGETS"))        io.testType = 5;
            else if (equalsNoCase(value, "CYCLE_OUTSTANDING_IOS"))            io.testType = 6;
            else if (equalsNoCase(value, "CYCLE_OUTSTANDING_IOS_AND_TARGETS")) io.testType = 7;
            // This value used to be stored as an integer.
            else if (!IcfStream::extractFirstInt(value, io.testType)) {
                m_errors.push_back("Error while reading file.  "
                    "For \"Test type\", expected a legal identifier or an "
                    "integer value.  See the documentation for details.");
                return false;
            }
        } else {   // Unrecognized comment
            if (version >= 19990217) {
                m_errors.push_back("File is improperly formatted.  TEST SETUP "
                    "section contained an unrecognized \"" + key + "\" comment.");
                return false;
            }
            // Versions as recent as 1998.10.08 have no 'End test setup.
            break;
        }
    }

    return true;
}

// ---- 'RESULTS DISPLAY (port of CPageDisplay::LoadConfig) -------------------------

bool IcfDocument::loadResultsDisplay(IcfDisplaySettings &io)
{
    m_errors.clear();
    IcfStream infile(m_path);

    const uint64_t version = infile.getVersion();
    if (version == IcfStream::BadVersion) {
        m_errors = infile.errors();
        return false;
    }
    if (!infile.skipTo("'RESULTS DISPLAY"))
        return true;   // no results display to restore (this is OK)

    std::string key, value;
    while (true) {
        if (!infile.getPair(key, value)) {
            m_errors.push_back("File is improperly formatted.  Expected results "
                               "display data or \"END results display\".");
            return false;
        }

        if (equalsNoCase(key, "'END results display")) {
            break;
        } else if (equalsNoCase(key, "'Record Last Update Results,Update Frequency,Update Type")) {
            const std::string recordTok = IcfStream::extractFirstToken(value);
            io.updateLinePresent = true;
            io.recordLastUpdate  = equalsNoCase(recordTok, "ENABLED");

            if (!IcfStream::extractFirstInt(value, io.updateFrequency)) {
                m_errors.push_back("Error while reading file.  "
                    "\"Update frequency\" should be specified as an integer value.");
                return false;
            }

            std::string updateType = value;
            trimRight(updateType);
            if (equalsNoCase(updateType, "LAST_UPDATE"))
                io.whichPerf = 1;                  // LAST_UPDATE_PERF
            else if (equalsNoCase(updateType, "WHOLE_TEST"))
                io.whichPerf = 0;                  // WHOLE_TEST_PERF
            else {
                m_errors.push_back("File is improperly formatted.  In RESULTS "
                    "DISPLAY section, Update Frequency was not followed by "
                    "an appropriate Update Type string.");
                return false;
            }
        } else if (startsWithNoCase(key, "'Bar chart")) {
            int barNumber = 0;
            if (!IcfStream::extractFirstInt(key, barNumber)) {
                m_errors.push_back("Error while reading file.  "
                    "\"Bar chart\" should be followed by an integer value.");
                return false;
            }

            std::string barItem = key;
            trimBoth(barItem);

            if (barNumber < 1 || barNumber > 6) {   // NUM_STATUS_BARS
                m_errors.push_back("Invalid bar chart number in RESULTS DISPLAY "
                    "section.  Ignoring this bar setting.");
                continue;
            }
            barNumber--;   // one-based in the file, zero-based internally

            IcfDisplaySettings::Bar bar;
            bar.index = barNumber;
            if (equalsNoCase(barItem, "statistic")) {
                bar.kind = IcfDisplaySettings::Bar::Statistic;
                bar.name = value;
                io.bars.push_back(bar);
            } else if (equalsNoCase(barItem, "manager ID, manager name")) {
                if (!IcfStream::extractFirstInt(value, bar.id)) {
                    m_errors.push_back("Error while reading file.  "
                        "Expected a manager ID integer after \"manager ID, manager name\""
                        "comment in RESULTS DISPLAY section.");
                    return false;
                }
                bar.kind = IcfDisplaySettings::Bar::Manager;
                bar.name = value;
                io.bars.push_back(bar);
            } else if (equalsNoCase(barItem, "worker ID, worker name")) {
                if (!IcfStream::extractFirstInt(value, bar.id)) {
                    m_errors.push_back("Error while reading file.  "
                        "Expected a worker ID integer after \"worker ID, worker name\""
                        "comment in RESULTS DISPLAY section.");
                    return false;
                }
                bar.kind = IcfDisplaySettings::Bar::Worker;
                bar.name = value;
                io.bars.push_back(bar);
            } else {
                m_errors.push_back("Invalid bar chart item name \"" + barItem
                    + "\" for bar #" + std::to_string(barNumber)
                    + " in RESULTS DISPLAY section.  Ignoring this bar setting.");
                continue;
            }
        } else {
            m_errors.push_back("File is improperly formatted.  RESULTS DISPLAY "
                "section contained an unrecognized \"" + key + "\" comment.");
            return false;
        }
    }

    return true;
}

uint64_t IcfDocument::readVersion() const
{
    IcfStream infile(m_path);
    return infile.getVersion();
}

// ---- 'MANAGER LIST (port of ManagerList/Manager/Worker::LoadConfig parsing) -----

namespace {

// Worker::LoadConfigDefault - the 'Default target settings for worker block.
// `errs` collects MFC error texts; returns false on a malformed/invalid block.
bool loadWorkerDefaults(IcfStream &infile, IcfWorkerConfig &w,
                        std::vector<std::string> &errs)
{
    w.hasDefaultSettings = true;
    std::string key, value, token;
    while (true) {
        if (!infile.getPair(key, value)) {
            errs.push_back("File is improperly formatted.  Expected more default target "
                "settings for worker or an \"End default target settings for worker\" comment.");
            return false;
        }
        if (equalsNoCase(key, "'End default target settings for worker"))
            return true;

        if (equalsNoCase(key, "'Number of outstanding IOs,test connection rate,transactions per connection")
            || equalsNoCase(key, "'Number of outstanding IOs,test connection rate,transactions per connection,use fixed seed,fixed seed value")) {
            const bool hasSeed = equalsNoCase(key,
                "'Number of outstanding IOs,test connection rate,transactions per connection,use fixed seed,fixed seed value");
            w.ioSettingsPresent = true;
            if (hasSeed) w.fixedSeedPresent = true;
            int n = 0;
            if (!IcfStream::extractFirstInt(value, n)) {
                errs.push_back("Error while reading file.  "
                    "\"Number of outstanding IOs\" should be specified as an integer value.");
                return false;
            }
            w.queueDepth = n;
            token = IcfStream::extractFirstToken(value);
            if (equalsNoCase(token, "ENABLED"))       w.testConnRate = true;
            else if (equalsNoCase(token, "DISABLED"))  w.testConnRate = false;
            else {
                errs.push_back("Error restoring worker " + w.name + ".  "
                    "\"Test connection rate\" should be set to ENABLED or DISABLED.");
                return false;
            }
            if (!IcfStream::extractFirstInt(value, n)) {
                errs.push_back("Error while reading file.  "
                    "\"Transactions per connection\" should be specified as an integer value.");
                return false;
            }
            w.transPerConn = n;
            if (hasSeed) {
                token = IcfStream::extractFirstToken(value);
                if (equalsNoCase(token, "ENABLED"))       w.useFixedSeed = true;
                else if (equalsNoCase(token, "DISABLED"))  w.useFixedSeed = false;
                else {
                    errs.push_back("Error restoring worker " + w.name + ".  "
                        "\"Use fixed seed\" should be set to ENABLED or DISABLED.");
                    return false;
                }
                uint64_t v64 = 0;
                if (!IcfStream::extractFirstUInt64(value, v64)) {
                    errs.push_back("Error while reading file.  "
                        "\"Fixed seed value\" should be specified as an integer value.");
                    return false;
                }
                w.fixedSeedValue = v64;
            }
        } else if (equalsNoCase(key, "'Disk maximum size,starting sector")
                || equalsNoCase(key, "'Disk maximum size,starting sector,Data pattern")) {
            const bool hasPattern = equalsNoCase(key, "'Disk maximum size,starting sector,Data pattern");
            w.diskSizePresent = true;
            if (hasPattern) w.dataPatternPresent = true;
            if (w.kind != IcfWorkerKind::Disk) {
                errs.push_back("Error restoring worker " + w.name + ".  "
                    "Cannot specify \"Disk maximum size,starting sector\" for a non-disk worker.");
                return false;
            }
            uint64_t v64 = 0;
            if (!IcfStream::extractFirstUInt64(value, v64)) {
                errs.push_back("Error while reading file.  "
                    "\"Disk maximum size\" should be specified as an integer value.");
                return false;
            }
            w.diskMaxSize = v64;
            if (!IcfStream::extractFirstUInt64(value, v64)) {
                errs.push_back("Error while reading file.  "
                    "\"Starting sector\" should be specified as an integer value.");
                return false;
            }
            w.startingSector = v64;
            if (hasPattern) {
                int n = 0;
                if (!IcfStream::extractFirstInt(value, n)) {
                    errs.push_back("Error while reading file.  "
                        "\"Data pattern\" should be specified as an integer value.");
                    return false;
                }
                w.dataPattern = n;
            }
        } else if (equalsNoCase(key, "'Local network interface")) {
            if (w.kind != IcfWorkerKind::NetTCP && w.kind != IcfWorkerKind::NetVI) {
                errs.push_back("Error restoring worker " + w.name + ".  "
                    "Cannot specify \"Local network interface\" for a non-TCP worker.");
                return false;
            }
            w.localNetIfacePresent = true;
            w.localNetworkInterface = value;
        } else if (equalsNoCase(key, "'VI outstanding IOs")) {
            if (w.kind != IcfWorkerKind::NetVI) {
                errs.push_back("Error restoring worker " + w.name + ".  "
                    "Cannot specify \"VI outstanding IOs\" for a non-VI worker.");
                return false;
            }
            int n = 0;
            if (!IcfStream::extractFirstInt(value, n)) {
                errs.push_back("Error while reading file.  "
                    "\"VI outstanding IOs\" should be specified as an integer value.");
                return false;
            }
            w.viIosPresent = true;
            w.viOutstandingIos = n;
        } else {
            errs.push_back("File is improperly formatted.  "
                "DEFAULT TARGET SETTINGS FOR WORKER section contained an unrecognized \""
                + key + "\"comment.");
            return false;
        }
    }
}

} // namespace

bool IcfDocument::loadManagerList(std::vector<IcfManagerConfig> &out,
                                  bool loadAspecs, bool loadTargets)
{
    m_errors.clear();
    IcfStream infile(m_path);

    if (infile.getVersion() == IcfStream::BadVersion) {
        m_errors = infile.errors();
        return false;
    }
    if (!infile.skipTo("'MANAGER LIST"))
        return true;   // no manager list to restore (this is OK)

    while (true) {
        const std::string key = infile.getNextLine();
        if (equalsNoCase(key, "'END manager list"))
            break;
        if (!equalsNoCase(key, "'Manager ID, manager name")) {
            m_errors.push_back("File is improperly formatted.  Expected another "
                "manager or \"End manager list\" comment.");
            return false;
        }

        // GetManagerInfo: "<id>,<name>" line, then 'Manager network address pair.
        IcfManagerConfig mgr;
        std::string idLine = infile.getNextLine();
        if (idLine.empty()) {
            m_errors.push_back("File is improperly formatted.  "
                "Error retrieving manager name or empty manager name.");
            return false;
        }
        if (!IcfStream::extractFirstInt(idLine, mgr.id)) {
            m_errors.push_back("File is improperly formatted.  "
                "Error retrieving manager ID.  This value must be an integer.");
            return false;
        }
        mgr.name = idLine;
        std::string k2, v2;
        if (!infile.getPair(k2, v2)) {
            m_errors.push_back("File is improperly formatted.  "
                "Error retrieving manager network address.");
            return false;
        }
        if (!equalsNoCase(k2, "'Manager network address")) {
            m_errors.push_back("File is improperly formatted.  Expected a "
                "\"Manager network address\" comment after manager ID.");
            return false;
        }
        mgr.address = v2;

        // Manager::LoadConfig - workers until 'End manager.
        // wkrType is function-scope in MFC (wkr_type_hex), so a bad NETWORK subtype
        // (which does not abort) leaves it at the previous worker's value.
        IcfWorkerKind wkrType = IcfWorkerKind::Invalid;
        while (true) {
            std::string wk, wv;
            if (!infile.getPair(wk, wv)) {
                m_errors.push_back("File is improperly formatted.  Expected a "
                    "worker or \"End manager\".");
                return false;
            }
            if (equalsNoCase(wk, "'End manager"))
                break;
            if (!equalsNoCase(wk, "'Worker")) {
                m_errors.push_back("File is improperly formatted.  MANAGER section "
                    "contained an unrecognized \"" + wk + "\" comment.");
                return false;
            }

            IcfWorkerConfig w;
            w.name = wv;

            std::string tk, tv;
            if (!infile.getPair(tk, tv)) {
                m_errors.push_back("File is improperly formatted.  Expected "
                    "\"Worker type\".");
                return false;
            }
            if (!equalsNoCase(tk, "'Worker type")) {
                m_errors.push_back("File is improperly formatted.  Worker name "
                    "should be followed by \"Worker type\" comment.");
                return false;
            }
            std::string tok = IcfStream::extractFirstToken(tv);
            if (equalsNoCase(tok, "DISK")) {
                wkrType = IcfWorkerKind::Disk;
            } else if (equalsNoCase(tok, "NETWORK")) {
                tok = IcfStream::extractFirstToken(tv);
                if (equalsNoCase(tok, "TCP"))      wkrType = IcfWorkerKind::NetTCP;
                else if (equalsNoCase(tok, "VI"))  wkrType = IcfWorkerKind::NetVI;
                else {
                    // MFC records the error but does NOT abort; wkr_type_hex keeps
                    // its prior value (InvalidType for the first such worker).
                    m_errors.push_back("Unknown network worker subtype encountered "
                        "for worker \"" + w.name + "\": \"" + tok
                        + "\".  Should be either TCP or VI.");
                }
            } else {
                m_errors.push_back("Unknown worker type encountered for worker \""
                    + w.name + "\": \"" + tok + "\".  Should be either DISK or NETWORK.");
                return false;
            }
            w.kind = wkrType;

            // Worker::LoadConfig - sub-sections via GetNextLine.
            while (true) {
                const std::string comment = infile.getNextLine();
                if (equalsNoCase(comment, "'End worker"))
                    break;
                if (equalsNoCase(comment, "'Default target settings for worker")) {
                    if (!loadWorkerDefaults(infile, w, m_errors))
                        return false;
                } else if (equalsNoCase(comment, "'Assigned access specs")) {
                    if (loadAspecs) {
                        while (true) {
                            const std::string sv = infile.getNextLine();
                            if (equalsNoCase(sv, "'End assigned access specs"))
                                break;
                            w.assignedSpecNames.push_back(sv);
                        }
                    } else if (!infile.skipTo("'End assigned access specs")) {
                        m_errors.push_back("File is improperly formatted.  Couldn't "
                            "find an \"End assigned access specs\" comment.");
                        return false;
                    }
                } else if (equalsNoCase(comment, "'Target assignments")) {
                    if (!loadTargets) {
                        if (!infile.skipTo("'End target assignments")) {
                            m_errors.push_back("File is improperly formatted.  Couldn't "
                                "find an \"End target assignments\" comment.");
                            return false;
                        }
                        continue;
                    }
                    while (true) {
                        std::string c, n;
                        if (!infile.getPair(c, n)) {
                            m_errors.push_back("File is improperly formatted.  Expected a "
                                "target or \"End target assignments\".");
                            return false;
                        }
                        if (equalsNoCase(c, "'End target assignments"))
                            break;
                        if (!equalsNoCase(c, "'Target")) {
                            m_errors.push_back("File is improperly formatted.  Expected "
                                "a \"Target\" comment inside TARGET ASSIGNMENTS section.");
                            return false;
                        }
                        IcfTargetConfig tgt;
                        tgt.name = n;
                        std::string tc, ts;
                        if (!infile.getPair(tc, ts)) {
                            m_errors.push_back("File is improperly formatted.  Expected "
                                "\"Target type\".");
                            return false;
                        }
                        if (!equalsNoCase(tc, "'Target type")) {
                            m_errors.push_back("File is improperly formatted.  Expected "
                                "a \"Target type\" comment after target name.");
                            return false;
                        }
                        std::string ttok = IcfStream::extractFirstToken(ts);
                        if (equalsNoCase(ttok, "DISK")) {
                            tgt.kind = IcfWorkerKind::Disk;
                            // Disk target: the 'End target line follows directly.
                            const std::string end = infile.getNextLine();
                            if (!equalsNoCase(end, "'End target")) {
                                m_errors.push_back("File is improperly formatted.  "
                                    "Expected an \"End target\" comment.");
                                return false;
                            }
                        } else if (equalsNoCase(ttok, "NETWORK")) {
                            ttok = IcfStream::extractFirstToken(ts);
                            if (equalsNoCase(ttok, "TCP"))      tgt.kind = IcfWorkerKind::NetTCP;
                            else if (equalsNoCase(ttok, "VI"))  tgt.kind = IcfWorkerKind::NetVI;
                            else {
                                m_errors.push_back("Error restoring target " + tgt.name
                                    + ".  Network target type is neither TCP nor VI.");
                                return false;
                            }
                            // A network target carries an extra "Target manager ID,
                            // manager name" pair (the remote manager that owns the
                            // NIC). We parse the identity here; the live NIC matching
                            // / CreateNetClient stays in the front-end adapter.
                            std::string mc, mi;
                            if (!infile.getPair(mc, mi)) {
                                m_errors.push_back("File is improperly formatted.  Expected "
                                    "network target manager ID and name.");
                                return false;
                            }
                            if (!equalsNoCase(mc, "'Target manager ID, manager name")) {
                                m_errors.push_back("File is improperly formatted.  Expected "
                                    "a \"Target manager ID, manager name\" comment after \"Target type\".");
                                return false;
                            }
                            if (!IcfStream::extractFirstInt(mi, tgt.targetManagerId)) {
                                m_errors.push_back("Error while reading file.  "
                                    "\"Target manager ID\" should be specified as an integer value.");
                                return false;
                            }
                            if (mi.empty()) {
                                m_errors.push_back("File is improperly formatted.  Expected "
                                    "a target \"manager name\".");
                                return false;
                            }
                            tgt.targetManagerName = mi;
                            // 'End target follows directly (MFC reads it via GetNextLine).
                            const std::string end = infile.getNextLine();
                            if (!equalsNoCase(end, "'End target")) {
                                m_errors.push_back("File is improperly formatted.  "
                                    "Expected an \"End target\" comment.");
                                return false;
                            }
                        } else {
                            m_errors.push_back("Error restoring target " + tgt.name
                                + ".  Target type is neither DISK nor NETWORK.");
                            return false;
                        }
                        w.targets.push_back(tgt);
                    }
                } else {
                    m_errors.push_back("File is improperly formatted.  WORKER section "
                        "contained an unrecognized line: \"" + comment + "\".");
                    return false;
                }
            }

            mgr.workers.push_back(w);
        }

        out.push_back(mgr);
    }

    return true;
}

// ---- 'ACCESS SPECIFICATIONS (port of AccessSpecList::LoadConfig{New,Old}) -------

namespace {

struct NamePool {
    const std::vector<std::string>      *existing;   // caller's live list (may be null)
    const std::vector<IcfAccessSpec>    *loaded;     // specs loaded so far
    const std::string                   *pending;    // current spec's name (may be null)

    bool contains(const std::string &name) const {
        if (existing)
            for (const auto &n : *existing)
                if (equalsNoCase(name, n.c_str())) return true;
        for (const auto &s : *loaded)
            if (equalsNoCase(name, s.name.c_str())) return true;
        if (pending && equalsNoCase(name, pending->c_str())) return true;
        return false;
    }
    // MFC NextUntitled: count "Untitled*" names, then increment until unique.
    std::string nextUntitled() const {
        int n = 0;
        if (existing)
            for (const auto &s : *existing) if (s.rfind("Untitled", 0) == 0) ++n;
        for (const auto &s : *loaded)       if (s.name.rfind("Untitled", 0) == 0) ++n;
        if (pending && pending->rfind("Untitled", 0) == 0) ++n;
        std::string name;
        do { name = "Untitled " + std::to_string(++n); } while (contains(name));
        return name;
    }
};

} // namespace

bool IcfDocument::loadAccessSpecs(std::vector<IcfAccessSpec> &out,
                                  const std::vector<std::string> *existingNames)
{
    m_errors.clear();
    IcfStream infile(m_path);

    const uint64_t version = infile.getVersion();
    if (version == IcfStream::BadVersion) {
        m_errors = infile.errors();
        return false;
    }
    if (!infile.skipTo("'ACCESS SPECIFICATIONS"))
        return true;   // no access spec list to restore (this is OK)

    std::ifstream &f = infile.raw();

    if (version >= 19980521) {
        // ---- modern named format (LoadConfigNew) -----------------------------
        std::string key, value;
        while (true) {
            if (!infile.getPair(key, value)) {
                m_errors.push_back("File is improperly formatted.  Expected an "
                    "access specification or \"END access specifications\".");
                return false;
            }
            if (equalsNoCase(key, "'END access specifications"))
                break;
            if (equalsNoCase(key, "'Access specification name,default assignment")) {
                // Replace-by-name: the lookup uses everything left of the comma.
                const std::size_t comma = value.find(',');
                const std::string lookup = (comma == std::string::npos)
                                           ? std::string() : value.substr(0, comma);
                std::size_t idx = out.size();
                for (std::size_t k = 0; k < out.size(); ++k) {
                    if (equalsNoCase(lookup, out[k].name.c_str())) { idx = k; break; }
                }
                if (idx == out.size())
                    out.push_back(IcfAccessSpec());
                IcfAccessSpec &spec = out[idx];
                spec.lines.clear();

                spec.name = IcfStream::extractFirstToken(value, true);

                const std::string token = IcfStream::extractFirstToken(value);
                if (equalsNoCase(token, "NONE"))      spec.defaultAssignment = 0;
                else if (equalsNoCase(token, "ALL"))  spec.defaultAssignment = 1;
                else if (equalsNoCase(token, "DISK")) spec.defaultAssignment = 2;
                else if (equalsNoCase(token, "NET"))  spec.defaultAssignment = 3;
                else spec.defaultAssignment = std::atoi(token.c_str());

                const std::string comment = infile.getNextLine();
                if (comment.empty() || comment[0] != '\'') {
                    m_errors.push_back("File is improperly formatted.  Expected a "
                        "comment line to follow the access specification name.");
                    return false;
                }

                // Data rows: 8 comma-separated fields, atoi'd (junk parses as 0).
                int total_access = 0;
                std::string str;
                while (f.peek() != '\'') {
                    IcfAccessSpecLine l;
                    std::getline(f, str, ','); l.size    = std::atoi(str.c_str());
                    std::getline(f, str, ','); l.ofSize  = std::atoi(str.c_str());
                    std::getline(f, str, ','); l.reads   = std::atoi(str.c_str());
                    std::getline(f, str, ','); l.random  = std::atoi(str.c_str());
                    std::getline(f, str, ','); l.delay   = std::atoi(str.c_str());
                    std::getline(f, str, ','); l.burst   = std::atoi(str.c_str());
                    std::getline(f, str, ','); l.align   = std::atoi(str.c_str());
                    std::getline(f, str);      l.reply   = std::atoi(str.c_str());

                    if (l.ofSize > 100 || l.reads > 100 || l.random > 100
                        || l.ofSize < 0 || l.reads < 0 || l.random < 0) {
                        out.erase(out.begin() + idx);   // MFC Delete(spec)
                        m_errors.push_back("Error loading global access specifications.  "
                                           "Invalid value encountered.");
                        return false;
                    }
                    total_access += l.ofSize;
                    spec.lines.push_back(l);
                }

                if (total_access != 100) {
                    m_errors.push_back("Error loading global access specifications.  "
                                       "Percentages don't add to 100.");
                    out.erase(out.begin() + idx);       // MFC Delete(spec)
                    return false;
                }
            } else {
                m_errors.push_back("File is improperly formatted.  Global ACCESS "
                    "SPECIFICATION section contained an unrecognized \"" + key + "\" comment.");
                return false;
            }
        }
        return true;
    }

    // ---- old numeric format, pre-1998.05.21 (LoadConfigOld) ---------------------
    infile.getNextLine();   // read and discard the initial comment line

    NamePool pool{ existingNames, &out, nullptr };
    IcfAccessSpec cur;
    cur.name = pool.nextUntitled();   // MFC New() names the spec immediately
    pool.pending = &cur.name;
    int total_access = 0;

    while (!f.rdstate()) {
        const int pk = f.peek();
        if (pk == '\'' || pk == EOF)
            return true;              // the trailing in-progress spec is dropped

        // InitAccessSpecLine defaults; only SIX fields are read in the old
        // format, so align stays 2048 and reply stays 0 (a faithful quirk).
        IcfAccessSpecLine l;
        l.size = 2048; l.ofSize = 100; l.reads = 67; l.random = 100;
        l.delay = 0;   l.burst = 1;    l.align = 2048; l.reply = 0;

        f >> l.size >> l.ofSize >> l.reads >> l.random >> l.delay >> l.burst;
        f.ignore(1, '\n');

        if (l.ofSize > 100 || l.reads > 100 || l.random > 100
            || l.ofSize < 0 || l.reads < 0 || l.random < 0)
            break;

        if (l.ofSize) {
            total_access += l.ofSize;
            cur.lines.push_back(l);
        } else {
            m_errors.push_back("Found an access specification line that makes up 0% of the test.  "
                               "This line will be discarded.");
        }

        if (total_access > 100)
            break;

        if (total_access == 100) {
            cur.defaultAssignment = 1;   // AssignAll

            // SmartName: single-line specs get a descriptive, unique name.
            if (cur.lines.size() == 1) {
                const IcfAccessSpecLine &sl = cur.lines[0];
                std::string name = smartNameText(sl.size, sl.random, sl.reads);
                const std::string base = name;
                int spec_number = 1;
                pool.pending = nullptr;   // MFC RefByName can't match the spec itself here
                while (pool.contains(name))
                    name = base + " " + std::to_string(++spec_number);
                cur.name = name;
            }

            out.push_back(cur);

            cur = IcfAccessSpec();
            pool.pending = nullptr;
            cur.name = pool.nextUntitled();
            pool.pending = &cur.name;
            total_access = 0;
        }
    }

    m_errors.push_back("Error loading access specification.  Invalid access specification.");
    return false;
}

} // namespace iocore
