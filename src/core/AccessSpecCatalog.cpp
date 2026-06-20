#include "AccessSpecCatalog.h"

namespace iocore {

std::string formatSize(int bytes)
{
    if (bytes <= 0)        return "0 B";
    if (bytes >= 1048576)  return std::to_string(bytes / 1048576) + " MiB";
    if (bytes >= 1024)     return std::to_string(bytes / 1024)    + " KiB";
    return                        std::to_string(bytes)            + " B";
}

std::string accessSpecLabel(const AccessSpec &spec)
{
    if (spec.lines.size() != 1)
        return spec.name;
    const AccessSpecLine &l = spec.lines[0];
    return formatSize(l.sizeBytes) + "; "
         + std::to_string(l.readPercent)       + "% Read; "
         + std::to_string(100 - l.seqPercent)  + "% random";
}

std::string smartNameText(int sizeBytes, int randomPct, int readPct)
{
    // Size part: "512 B" / "1 MiB" / "N KiB".
    std::string name;
    if (sizeBytes == 512)            name = "512 B";
    else if (sizeBytes == 1048576)   name = "1 MiB";
    else                             name = std::to_string(sizeBytes / 1024) + " KiB";

    // Random/sequential part.
    if (randomPct == 0)         name += " sequential";
    else if (randomPct == 100)  name += " random";
    else                        name += " " + std::to_string(randomPct) + "% random";

    // Read/write part.
    if (readPct == 0)         name += " writes";
    else if (readPct == 100)  name += " reads";
    else                      name += " " + std::to_string(readPct) + "% reads";

    return name;
}

// Helper: a single I/O pattern row.
static AccessSpecLine line(int sizeBytes, int readPct, int seqPct,
                           int alignBytes = 0, int ofSize = 100)
{
    AccessSpecLine l;
    l.sizeBytes   = sizeBytes;
    l.readPercent = readPct;
    l.seqPercent  = seqPct;
    l.alignBytes  = alignBytes;
    l.ofSize      = ofSize;
    return l;
}

std::vector<AccessSpec> defaultAccessSpecs()
{
    std::vector<AccessSpec> specs;

    auto addOne = [&](const std::string &name, const AccessSpecLine &l) {
        AccessSpec s;
        s.name = name;
        s.lines.push_back(l);
        specs.push_back(s);
    };

    // -- Idle (size = 0) --------------------------------------------------------
    {
        AccessSpec s;
        s.name = "Idle";
        AccessSpecLine l;
        l.sizeBytes = 0;
        l.ofSize    = 100;
        s.lines.push_back(l);
        specs.push_back(s);
    }

    // -- Default (2 KiB, 67% read, 100% random, request-size aligned) ----------
    // This is the canonical MFC default (AccessSpecList::InitAccessSpecLine):
    // size 2048, 67% reads, fully random, aligned to the request size (2048).
    {
        AccessSpec s;
        s.name        = "Default";
        s.defaultSpec = true;
        s.lines.push_back(line(2048, 67, 0, 2048));
        specs.push_back(s);
    }

    // -- All patterns, in order (also used to build "All in one") --------------
    struct Pat { const char *name; AccessSpecLine l; };
    const std::vector<Pat> patterns = {
        { "512 B; 100% Read; 0% random",  line(512, 100, 100) },
        { "512 B; 75% Read; 0% random",   line(512,  75, 100) },
        { "512 B; 50% Read; 0% random",   line(512,  50, 100) },
        { "512 B; 25% Read; 0% random",   line(512,  25, 100) },
        { "512 B; 0% Read; 0% random",    line(512,   0, 100) },
        { "4 KiB; 100% Read; 0% random",  line(4096, 100, 100) },
        { "4 KiB; 75% Read; 0% random",   line(4096,  75, 100) },
        { "4 KiB; 50% Read; 0% random",   line(4096,  50, 100) },
        { "4 KiB; 25% Read; 0% random",   line(4096,  25, 100) },
        { "4 KiB; 0% Read; 0% random",    line(4096,   0, 100) },
        { "4 KiB aligned; 100% Read; 100% random", line(4096, 100, 0, 4096) },
        { "4 KiB aligned; 50% Read; 100% random",  line(4096,  50, 0, 4096) },
        { "4 KiB aligned; 0% Read; 100% random",   line(4096,   0, 0, 4096) },
        { "16 KiB; 100% Read; 0% random", line(16384, 100, 100) },
        { "16 KiB; 75% Read; 0% random",  line(16384,  75, 100) },
        { "16 KiB; 50% Read; 0% random",  line(16384,  50, 100) },
        { "16 KiB; 25% Read; 0% random",  line(16384,  25, 100) },
        { "16 KiB; 0% Read; 0% random",   line(16384,   0, 100) },
        { "32 KiB; 100% Read; 0% random", line(32768, 100, 100) },
        { "32 KiB; 75% Read; 0% random",  line(32768,  75, 100) },
        { "32 KiB; 50% Read; 0% random",  line(32768,  50, 100) },
        { "32 KiB; 25% Read; 0% random",  line(32768,  25, 100) },
        { "32 KiB; 0% Read; 0% random",   line(32768,   0, 100) },
        { "64 KiB; 100% Read; 0% random", line(65536, 100, 100) },
        { "64 KiB; 50% Read; 0% random",  line(65536,  50, 100) },
        { "64 KiB; 0% Read; 0% random",   line(65536,   0, 100) },
        { "256 KiB; 100% Read; 0% random", line(262144, 100, 100) },
        { "256 KiB; 50% Read; 0% random",  line(262144,  50, 100) },
        { "256 KiB; 0% Read; 0% random",   line(262144,   0, 100) },
    };

    for (const auto &p : patterns)
        addOne(p.name, p.l);

    // -- "All in one" - every pattern with an even % access distribution -------
    {
        AccessSpec s;
        s.name = "All in one";
        const int n = static_cast<int>(patterns.size());
        for (int i = 0; i < n; ++i) {
            AccessSpecLine l = patterns[i].l;
            // Distribute 100% as evenly as possible (matches the original rounding).
            l.ofSize = (i < 100 % n) ? 100 / n + 1 : 100 / n;
            s.lines.push_back(l);
        }
        specs.push_back(s);
    }

    return specs;
}

} // namespace iocore
