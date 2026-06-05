#include "IcfFile.h"
#include <fstream>
#include <sstream>
#include <algorithm>

const std::string IcfFile::VERSION_STRING = "1.1.0";

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
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        lines.push_back(line);
    }
    f.close();

    cfg = TestConfig();
    specs.clear();
    workers.clear();

    // Basic parsing - minimal implementation for now
    // Full parsing would match original DynamoEngine::loadConfig logic
    for (size_t i = 0; i < lines.size(); ++i) {
        const auto &t = lines[i];

        // Test Setup section
        if (t == "'Run Time") {
            // Parse hours, minutes, seconds (simplified)
            // Would need proper line parsing here
        }
        // More sections would go here
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
    out << "\t" << cfg.runHours << "          " << cfg.runMinutes << "          " << cfg.runSeconds << "\n";
    out << "'Ramp Up Time (s)\n";
    out << "\t" << cfg.rampSeconds << "\n";

    // More sections would go here

    out << "'END manager list\n";
    out << "Version " << VERSION_STRING << " \n";
    out.close();
    return true;
}

bool IcfFile::isValidVersion(const std::string &versionStr)
{
    return versionStr.find("1.1.0") != std::string::npos;
}
