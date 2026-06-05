#include "IcfFile.h"
#include "../qt/IometerTypes.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

const QString IcfFile::VERSION_STRING = "1.1.0";

bool IcfFile::load(const QString &filepath,
                   TestConfig &cfg,
                   QList<AccessSpec> &specs,
                   QList<BatchWorker> &workers)
{
    QFile f(filepath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QStringList lines;
    QTextStream in(&f);
    while (!in.atEnd()) lines.append(in.readLine());
    f.close();

    cfg = TestConfig();
    specs.clear();
    workers.clear();

    // Helper: get first non-comment, non-empty data line after index i
    auto dataAfter = [&](int i) -> QString {
        for (int j = i + 1; j < lines.size(); ++j) {
            QString t = lines[j].trimmed();
            if (t.isEmpty() || t.startsWith("'")) continue;
            return t;
        }
        return {};
    };

    for (int i = 0; i < lines.size(); ++i) {
        const QString t = lines[i].trimmed();

        // -- Test Setup section -------------------------------------------------
        if (t == "'Run Time") {
            QString data;
            for (int j = i + 1; j < lines.size(); ++j) {
                QString d = lines[j].trimmed();
                if (d.isEmpty()) continue;
                if (d.startsWith("'")) continue;
                data = d; break;
            }
            const QStringList p = data.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (p.size() >= 3) {
                cfg.runHours   = p[0].toInt();
                cfg.runMinutes = p[1].toInt();
                cfg.runSeconds = p[2].toInt();
            }
        }
        else if (t == "'Ramp Up Time (s)") {
            const QString data = dataAfter(i);
            cfg.rampSeconds = data.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).value(0).toInt();
        }
        else if (t == "'Test Description") {
            if (i + 1 < lines.size())
                cfg.description = lines[i + 1].trimmed();
        }

        // -- Access Specifications section ------------------------------------
        else if (t.startsWith("'Access specification name")) {
            AccessSpec spec;
            int j = i + 1;

            // Find spec name (first data line)
            while (j < lines.size()) {
                const QString d = lines[j++].trimmed();
                if (d.isEmpty() || d.startsWith("'")) continue;
                spec.name = d.split(',').value(0).trimmed();
                break;
            }

            // Collect all spec data lines
            while (j < lines.size()) {
                const QString d = lines[j].trimmed();
                if (d.startsWith("'Access specification name") ||
                    d.startsWith("'END access")) break;
                ++j;
                if (d.isEmpty()) continue;
                if (d.startsWith("'")) continue;

                // Data line: size,%ofSize,%reads,%random,delay,burst,align,reply
                const QStringList p = d.split(',');
                if (p.size() >= 8) {
                    AccessSpecLine l;
                    l.sizeBytes   = p[0].toInt();
                    l.ofSize      = p[1].toInt();
                    l.readPercent = p[2].toInt();
                    l.seqPercent  = 100 - p[3].toInt();  // %random → %sequential
                    l.delayMs     = p[4].toInt();
                    l.burstLength = p[5].toInt();
                    l.alignBytes  = p[6].toInt();
                    l.replyBytes  = p[7].toInt();
                    spec.lines.append(l);
                }
            }

            if (!spec.name.isEmpty() && !spec.lines.isEmpty())
                specs.append(spec);
        }

        // -- Manager / Worker section -------------------------------------------
        else if (t == "'Worker") {
            BatchWorker bw;
            bw.name = dataAfter(i);

            // Scan forward for assigned specs and targets
            for (int j = i + 1; j < lines.size(); ++j) {
                const QString d = lines[j].trimmed();
                if (d == "'End worker") { i = j; break; }
                if (d == "'Assigned access specs") {
                    for (int k = j + 1; k < lines.size(); ++k) {
                        const QString s = lines[k].trimmed();
                        if (s.startsWith("'End assigned access")) { j = k; break; }
                        if (!s.isEmpty() && !s.startsWith("'"))
                            bw.assignedSpecs.append(s);
                    }
                }
                else if (d == "'Target") {
                    const QString tgt = dataAfter(j);
                    if (!tgt.isEmpty()) bw.targets.append(tgt);
                }
            }

            if (!bw.name.isEmpty())
                workers.append(bw);
        }
    }

    return true;
}

bool IcfFile::save(const QString &filepath,
                   const TestConfig &cfg,
                   const QList<AccessSpec> &specs,
                   const QList<BatchWorker> &workers)
{
    QFile f(filepath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&f);

    out << "Version 1.1.0 \n";

    // -- Test Setup section --------------------------------------------------
    out << "'TEST SETUP ====================================================================\n";
    out << "'Test Description\n";
    out << "\t" << cfg.description << "\n";
    out << "'Run Time\n";
    out << "'\thours      minutes    seconds\n";
    out << "\t" << cfg.runHours
        << "          " << cfg.runMinutes
        << "          " << cfg.runSeconds << "\n";
    out << "'Ramp Up Time (s)\n";
    out << "\t" << cfg.rampSeconds << "\n";
    out << "'Default Disk Workers to Spawn\n\tNUMBER_OF_CPUS\n";
    out << "'Default Network Workers to Spawn\n\t0\n";
    out << "'Record Results\n\tALL\n";
    out << "'Worker Cycling\n'\tstart      step       step type\n\t1          1          LINEAR\n";
    out << "'Disk Cycling\n'\tstart      step       step type\n\t1          1          LINEAR\n";
    out << "'Queue Depth Cycling\n'\tstart      end        step       step type\n\t1          32         2          EXPONENTIAL\n";
    out << "'Test Type\n\tNORMAL\n";
    out << "'END test setup\n";

    // -- Results Display section -----------------------------------------------
    out << "'RESULTS DISPLAY ===============================================================\n";
    out << "'Record Last Update Results,Update Frequency,Update Type\n";
    out << "\tDISABLED,1,LAST_UPDATE\n";
    out << "'Bar chart 1 statistic\n\tTotal I/Os per Second\n";
    out << "'Bar chart 2 statistic\n\tTotal MBs per Second (Decimal)\n";
    out << "'Bar chart 3 statistic\n\tAverage I/O Response Time (ms)\n";
    out << "'Bar chart 4 statistic\n\tMaximum I/O Response Time (ms)\n";
    out << "'Bar chart 5 statistic\n\t% CPU Utilization (total)\n";
    out << "'Bar chart 6 statistic\n\tTotal Error Count\n";
    out << "'END results display\n";

    // -- Access Specifications section ----------------------------------------
    out << "'ACCESS SPECIFICATIONS =========================================================\n";
    for (const auto &s : specs) {
        out << "'Access specification name,default assignment\n";
        out << "\t" << s.name << ",NONE\n";
        out << "'size,% of size,% reads,% random,delay,burst,align,reply\n";
        for (const auto &l : s.lines) {
            out << "\t" << l.sizeBytes
                << "," << l.ofSize
                << "," << l.readPercent
                << "," << (100 - l.seqPercent)   // seqPct → randomPct
                << "," << l.delayMs
                << "," << l.burstLength
                << "," << l.alignBytes
                << "," << l.replyBytes << "\n";
        }
    }
    out << "'END access specifications\n";

    // -- Manager List section -------------------------------------------------
    out << "'MANAGER LIST ==================================================================\n";

    auto writeWorker = [&](const QString &name, const QString &type,
                           const QStringList &assignedSpecs,
                           const QStringList &targets) {
        out << "'Worker\n\t" << name << "\n";
        out << "'Worker type\n\t" << type << "\n";
        out << "'Default target settings for worker\n";
        out << "'Number of outstanding IOs,test connection rate,transactions per connection,use fixed seed,fixed seed value\n";
        out << "\t1,DISABLED,1,DISABLED,0\n";
        out << "'Disk maximum size,starting sector,Data pattern\n";
        out << "\t0,0,0\n";
        out << "'End default target settings for worker\n";
        out << "'Assigned access specs\n";
        for (const auto &sn : assignedSpecs) out << "\t" << sn << "\n";
        if (assignedSpecs.isEmpty() && !specs.isEmpty())
            out << "\t" << specs.first().name << "\n";
        out << "'End assigned access specs\n";
        out << "'Target assignments\n";
        for (const auto &tgt : targets) {
            out << "'Target\n\t" << tgt << "\n";
            out << "'Target type\n\tDISK\n";
            out << "'End target\n";
        }
        out << "'End target assignments\n";
        out << "'End worker\n";
    };

    out << "'Manager ID, manager name\n\t1,MANAGER\n";
    out << "'Manager network address\n\t\n";

    if (workers.isEmpty() && !specs.isEmpty()) {
        // No batch workers - write a default one with first spec
        writeWorker("Worker 1", "DISK", {specs.first().name}, {});
    } else {
        // Write configured batch workers
        for (const auto &bw : workers) {
            writeWorker(bw.name.isEmpty() ? "Worker 1" : bw.name, "DISK",
                       bw.assignedSpecs, bw.targets);
        }
    }

    out << "'End manager\n";
    out << "'END manager list\n";
    out << "Version 1.1.0 \n";

    f.close();
    return true;
}

bool IcfFile::isValidVersion(const QString &versionStr)
{
    return versionStr.contains("1.1.0");
}
