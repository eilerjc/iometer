#include "IcfFile.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

const std::string IcfFile::VERSION_STRING = "1.1.0";

// --- Parsing helpers ---------------------------------------------------------

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, delim)) {
        std::string trimmed = trim(part);
        if (!trimmed.empty()) {
            parts.push_back(trimmed);
        }
    }
    return parts;
}

// Split by whitespace (one or more spaces/tabs)
static std::vector<std::string> splitWhitespace(const std::string &s) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string part;
    while (ss >> part) {
        parts.push_back(part);
    }
    return parts;
}

static int parseInt(const std::string &s) {
    try {
        return std::stoi(s);
    } catch (...) {
        return 0;
    }
}

static double parseDouble(const std::string &s) {
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0;
    }
}

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
    std::ifstream f(filepath);
    if (!f) return false;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) {
        lines.push_back(trim(line));
    }
    f.close();

    cfg = TestConfig();
    specs.clear();
    workers.clear();

    size_t i = 0;

    // Parse Version
    if (i < lines.size() && lines[i].find("Version") == 0) {
        i++;
    }

    // Parse TEST SETUP section
    while (i < lines.size()) {
        const auto &t = lines[i];
        if (t == "'Test Description") {
            ++i;
            if (i < lines.size()) {
                cfg.description = lines[i];
                ++i;
            }
        } else if (t == "'Run Time") {
            ++i;  // skip "'Run Time" line
            if (i < lines.size() && lines[i].find("hours") != std::string::npos) {
                ++i;  // skip comment line "hours  minutes  seconds"
            }
            if (i < lines.size()) {
                auto parts = splitWhitespace(lines[i]);
                if (parts.size() >= 3) {
                    cfg.runHours = parseInt(parts[0]);
                    cfg.runMinutes = parseInt(parts[1]);
                    cfg.runSeconds = parseInt(parts[2]);
                }
                ++i;
            }
        } else if (t == "'Ramp Up Time (s)") {
            ++i;
            if (i < lines.size() && !lines[i].empty() && lines[i][0] != '\'') {
                auto parts = splitWhitespace(lines[i]);
                if (!parts.empty()) {
                    cfg.rampSeconds = parseInt(parts[0]);
                }
                ++i;
            }
        } else if (t == "'Record Results") {
            ++i;
            if (i < lines.size()) {
                std::string val = lines[i];
                cfg.recordResults = (val == "ALL") ? 0 : 1;
                ++i;
            }
        } else if (t == "'Worker Cycling") {
            ++i;
            if (i < lines.size() && lines[i].find("start") != std::string::npos) {
                ++i;  // skip comment line
            }
            if (i < lines.size()) {
                ++i;  // skip data line
            }
        } else if (t == "'Disk Cycling") {
            ++i;
            if (i < lines.size() && lines[i].find("start") != std::string::npos) {
                ++i;  // skip comment line
            }
            if (i < lines.size()) {
                ++i;  // skip data line
            }
        } else if (t == "'Queue Depth Cycling") {
            ++i;
            if (i < lines.size() && lines[i].find("start") != std::string::npos) {
                ++i;  // skip comment line
            }
            if (i < lines.size()) {
                auto parts = splitWhitespace(lines[i]);
                if (parts.size() >= 3) {
                    cfg.ioqStart = parseInt(parts[0]);
                    cfg.ioqEnd = parseInt(parts[1]);
                    cfg.ioqPower = parseInt(parts[2]);
                }
                ++i;
            }
        } else if (t == "'Test Type") {
            ++i;
            if (i < lines.size()) {
                // Parse test type
                ++i;
            }
        } else if (t == "'END test setup") {
            ++i;
            break;
        } else if (t.find("'Default") == 0 || t.find("'Number of") == 0) {
            ++i;
        } else if (!t.empty() && t[0] != '\'') {
            // Data line under a known section - skip
            ++i;
        } else {
            ++i;
        }
    }

    // Skip RESULTS DISPLAY section
    while (i < lines.size() && lines[i] != "'END results display") {
        ++i;
    }
    if (i < lines.size()) ++i;  // skip END marker

    // Parse ACCESS SPECIFICATIONS section
    while (i < lines.size() && lines[i] != "'MANAGER LIST ==================================================================") {
        if (lines[i] == "'Access specification name,default assignment") {
            ++i;
            if (i < lines.size()) {
                AccessSpec spec;
                auto parts = split(lines[i], ',');
                if (!parts.empty()) {
                    spec.name = parts[0];
                    if (parts.size() > 1 && parts[1].find("DEFAULT") != std::string::npos) {
                        spec.defaultSpec = true;
                    }
                }
                ++i;

                // Parse access lines until we hit a comment or another spec name
                while (i < lines.size() && !lines[i].empty() && lines[i][0] != '\'' &&
                       lines[i].find("END") == std::string::npos) {
                    auto fields = split(lines[i], ',');
                    if (fields.size() >= 4) {
                        AccessSpecLine asl;
                        asl.sizeBytes = parseInt(fields[0]);
                        asl.ofSize = parseInt(fields[1]);
                        asl.readPercent = parseInt(fields[2]);
                        // field[3] is % random; seqPercent = 100 - randomPercent
                        int randomPercent = parseInt(fields[3]);
                        asl.seqPercent = 100 - randomPercent;
                        if (fields.size() > 4) asl.delayMs = parseDouble(fields[4]);
                        if (fields.size() > 5) asl.burstLength = parseInt(fields[5]);
                        if (fields.size() > 6) asl.alignBytes = parseInt(fields[6]);
                        if (fields.size() > 7) asl.replyBytes = parseInt(fields[7]);
                        spec.lines.push_back(asl);
                    }
                    ++i;
                }

                if (!spec.lines.empty()) {
                    specs.push_back(spec);
                }
            }
        } else if (lines[i] == "'END access specifications") {
            ++i;
            break;
        } else {
            ++i;
        }
    }

    // Parse MANAGER LIST section
    // Manager list format:
    // 'Manager ID, manager name
    //   <id>,<name>
    // 'Worker
    //   <worker name>
    // 'Assigned access specs
    //   <spec name>
    // 'Target assignments
    // 'Target
    //   <target name>
    // ...
    while (i < lines.size() && lines[i] != "'END manager list") {
        if (lines[i] == "'Manager ID, manager name") {
            ++i;
            if (i < lines.size()) {
                // Manager definition - not used in batch mode
                ++i;
            }
        } else if (lines[i] == "'Worker") {
            ++i;
            if (i < lines.size()) {
                BatchWorker bw;
                bw.name = lines[i];  // worker name
                ++i;

                // Parse worker details
                while (i < lines.size() && lines[i] != "'End worker") {
                    if (lines[i] == "'Assigned access specs") {
                        ++i;
                        if (i < lines.size() && lines[i] != "'End assigned access specs") {
                            bw.assignedSpecs.push_back(lines[i]);
                            ++i;
                        }
                        while (i < lines.size() && lines[i] != "'End assigned access specs") {
                            ++i;
                        }
                        if (i < lines.size()) ++i;
                    } else if (lines[i] == "'Target assignments") {
                        ++i;
                        while (i < lines.size() && lines[i] != "'End target assignments") {
                            if (lines[i] == "'Target") {
                                ++i;
                                if (i < lines.size() && lines[i] != "'Target type") {
                                    bw.targets.push_back(extractTarget(lines[i]));
                                    ++i;
                                }
                            } else if (lines[i] == "'Target type") {
                                ++i;
                                if (i < lines.size()) ++i;  // skip type
                            } else if (lines[i] == "'End target") {
                                ++i;
                            } else {
                                ++i;
                            }
                        }
                        if (i < lines.size()) ++i;
                    } else if (!lines[i].empty() && lines[i][0] != '\'') {
                        ++i;  // skip data lines
                    } else {
                        ++i;
                    }
                }
                if (i < lines.size()) ++i;  // skip "'End worker"

                if (!bw.name.empty()) {
                    workers.push_back(bw);
                }
            }
        } else if (lines[i] == "'Manager network address") {
            ++i;
            if (i < lines.size()) ++i;
        } else if (lines[i] == "'End manager") {
            ++i;
        } else {
            ++i;
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

    out << "Version " << VERSION_STRING << " \n";
    out << "'TEST SETUP ====================================================================\n";
    out << "'Test Description\n";
    out << "\t" << cfg.description << "\n";
    out << "'Run Time\n";
    out << "'\thours      minutes    seconds\n";
    out << "\t" << cfg.runHours << "\t" << cfg.runMinutes << "\t" << cfg.runSeconds << "\n";
    out << "'Ramp Up Time (s)\n";
    out << "\t" << cfg.rampSeconds << "\n";
    out << "'Default Disk Workers to Spawn\n";
    out << "\t1\n";
    out << "'Default Network Workers to Spawn\n";
    out << "\t0\n";
    out << "'Record Results\n";
    out << "\t" << (cfg.recordResults == 0 ? "ALL" : "LAST_UPDATE") << "\n";
    out << "'Worker Cycling\n";
    out << "'\tstart      step       step type\n";
    out << "\t" << cfg.workerStart << "\t" << cfg.workerStep << "\tLINEAR\n";
    out << "'Disk Cycling\n";
    out << "'\tstart      step       step type\n";
    out << "\t" << cfg.targetStart << "\t" << cfg.targetStep << "\tLINEAR\n";
    out << "'Queue Depth Cycling\n";
    out << "'\tstart      end        step       step type\n";
    out << "\t" << cfg.ioqStart << "\t" << cfg.ioqEnd << "\t" << cfg.ioqPower << "\tEXPONENTIAL\n";
    out << "'Test Type\n";
    out << "\tNORMAL\n";
    out << "'END test setup\n";

    // RESULTS DISPLAY section
    out << "'RESULTS DISPLAY ===============================================================\n";
    out << "'Record Last Update Results,Update Frequency,Update Type\n";
    out << "\tDISABLED,1,LAST_UPDATE\n";
    out << "'Bar chart 1 statistic\n";
    out << "\tTotal I/Os per Second\n";
    out << "'Bar chart 2 statistic\n";
    out << "\tTotal MBs per Second (Decimal)\n";
    out << "'Bar chart 3 statistic\n";
    out << "\tAverage I/O Response Time (ms)\n";
    out << "'Bar chart 4 statistic\n";
    out << "\tMaximum I/O Response Time (ms)\n";
    out << "'Bar chart 5 statistic\n";
    out << "\t% CPU Utilization (total)\n";
    out << "'Bar chart 6 statistic\n";
    out << "\tTotal Error Count\n";
    out << "'END results display\n";

    // ACCESS SPECIFICATIONS section
    out << "'ACCESS SPECIFICATIONS =========================================================\n";
    for (const auto &spec : specs) {
        out << "'Access specification name,default assignment\n";
        out << "\t" << spec.name << (spec.defaultSpec ? ",DEFAULT" : ",NONE") << "\n";
        out << "'size,% of size,% reads,% random,delay,burst,align,reply\n";
        for (const auto &line : spec.lines) {
            int randomPercent = 100 - line.seqPercent;
            out << "\t" << line.sizeBytes << "," << line.ofSize << ","
                << line.readPercent << "," << randomPercent << ","
                << static_cast<int>(line.delayMs) << "," << line.burstLength << ","
                << line.alignBytes << "," << line.replyBytes << "\n";
        }
    }
    out << "'END access specifications\n";

    // MANAGER LIST section
    out << "'MANAGER LIST ==================================================================\n";
    if (!workers.empty()) {
        out << "'Manager ID, manager name\n";
        out << "\t1,TESTHOST\n";
        out << "'Manager network address\n";
        out << "\t\n";
        for (const auto &bw : workers) {
            out << "'Worker\n";
            out << "\t" << bw.name << "\n";
            out << "'Worker type\n";
            out << "\tDISK\n";
            out << "'Default target settings for worker\n";
            out << "'Number of outstanding IOs,test connection rate,transactions per connection,use fixed seed,fixed seed value\n";
            out << "\t1,DISABLED,1,DISABLED,0\n";
            out << "'Disk maximum size,starting sector,Data pattern\n";
            out << "\t0,0,0\n";
            out << "'End default target settings for worker\n";
            out << "'Assigned access specs\n";
            for (const auto &spec : bw.assignedSpecs) {
                out << "\t" << spec << "\n";
            }
            out << "'End assigned access specs\n";
            out << "'Target assignments\n";
            for (const auto &target : bw.targets) {
                out << "'Target\n";
                out << "\t" << target << ": \"System\"\n";
                out << "'Target type\n";
                out << "\tDISK\n";
                out << "'End target\n";
            }
            out << "'End target assignments\n";
            out << "'End worker\n";
        }
        out << "'End manager\n";
    }
    out << "'END manager list\n";
    out << "Version " << VERSION_STRING << " \n";
    out.close();
    return true;
}

bool IcfFile::isValidVersion(const std::string &versionStr)
{
    return versionStr.find("1.1.0") != std::string::npos;
}
