#include "IcfFile.h"
#include "../IometerTypes.h"

#include <QFile>
#include <QTextStream>

const QString IcfFile::VERSION_STRING = "1.1.0";

bool IcfFile::load(const QString &filepath, TestConfig &cfg)
{
    QFile f(filepath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&f);
    bool inTestSetup = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip empty and comment lines
        if (line.isEmpty() || line.startsWith('\'')) {
            continue;
        }

        // Look for test setup section
        if (line.contains("Test Setup", Qt::CaseInsensitive)) {
            inTestSetup = true;
            continue;
        }

        if (inTestSetup) {
            // Parse run time fields
            if (line.contains("Run Time", Qt::CaseInsensitive)) {
                // Next line has: hours  minutes  seconds
                QString timeLine = in.readLine();
                QStringList parts = timeLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (parts.size() >= 3) {
                    cfg.runHours = parts[0].toInt();
                    cfg.runMinutes = parts[1].toInt();
                    cfg.runSeconds = parts[2].toInt();
                }
                continue;
            }

            if (line.contains("Ramp", Qt::CaseInsensitive)) {
                QStringList parts = line.split('\t', Qt::SkipEmptyParts);
                if (parts.size() > 1) {
                    cfg.rampSeconds = parts[1].toInt();
                }
                continue;
            }

            if (line.contains("Description", Qt::CaseInsensitive)) {
                QString desc = in.readLine().trimmed();
                cfg.description = desc;
                continue;
            }
        }

        // More ICF parsing would go here...
        // For now, return true to indicate basic parsing succeeded
    }

    f.close();
    return true;
}

bool IcfFile::save(const QString &filepath, const TestConfig &cfg)
{
    QFile f(filepath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&f);

    // Write ICF header
    out << "'Iometer Configuration File\n";
    out << "Version " << VERSION_STRING << "\n";

    // Write test setup section
    out << "'Test Setup\n";
    out << "'Run Time\n'\thours      minutes    seconds\n";
    out << "\t" << cfg.runHours
        << "          " << cfg.runMinutes
        << "          " << cfg.runSeconds << "\n";
    out << "'Ramp Up Time (s)\n\t" << cfg.rampSeconds << "\n";

    if (!cfg.description.isEmpty()) {
        out << "'Description\n\t" << cfg.description << "\n";
    }

    // More ICF writing would go here...

    f.close();
    return true;
}

bool IcfFile::isValidVersion(const QString &versionStr)
{
    return versionStr.trimmed() == VERSION_STRING;
}
