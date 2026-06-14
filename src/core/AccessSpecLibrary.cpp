#include "AccessSpecLibrary.h"
#include "AccessSpecCatalog.h"
#include <cctype>

namespace iocore {

// Case-insensitive compare (MFC used strcasecmp on the spec names).
static bool iequals(const std::string &a, const std::string &b)
{
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

AccessSpecLine AccessSpecLibrary::defaultLine()
{
    // MFC InitAccessSpecLine: size 2048, of_size 100, random 100, reads 67,
    // delay 0, burst 1, align = size (explicit 2048 on the wire), reply 0.
    AccessSpecLine l;
    l.sizeBytes   = 2048;
    l.ofSize      = 100;
    l.readPercent = 67;
    l.seqPercent  = 0;      // random 100
    l.delayMs     = 0.0;
    l.burstLength = 1;
    l.alignBytes  = 2048;   // align = size
    l.replyBytes  = 0;
    return l;
}

void AccessSpecLibrary::loadDefaults()
{
    m_specs = defaultAccessSpecs();   // Idle + Default + matrix + "All in one"
}

int AccessSpecLibrary::newSpec()
{
    AccessSpec s;
    s.name        = nextUntitledName();
    s.defaultSpec = false;
    s.lines.push_back(defaultLine());
    m_specs.push_back(std::move(s));
    return count() - 1;
}

int AccessSpecLibrary::copySpec(int index)
{
    if (index < 0 || index >= count())
        return -1;

    AccessSpec copy = m_specs[index];   // copy BEFORE push_back may reallocate

    // "Copy of <name> (n)" with the first free n (MFC always suffixes "(1)" up).
    const std::string base = "Copy of " + copy.name;
    int n = 1;
    std::string name;
    do {
        name = base + " (" + std::to_string(n++) + ")";
    } while (indexByName(name) != -1);
    copy.name = name;

    m_specs.push_back(std::move(copy));
    return count() - 1;
}

bool AccessSpecLibrary::removeAt(int index)
{
    if (index < 0 || index >= count())
        return false;
    m_specs.erase(m_specs.begin() + index);
    return true;
}

int AccessSpecLibrary::indexByName(const std::string &name) const
{
    for (int i = 0; i < count(); ++i) {
        if (iequals(m_specs[i].name, name))
            return i;
    }
    return -1;
}

std::string AccessSpecLibrary::nextUntitledName() const
{
    // Count existing "Untitled*" specs, then keep incrementing until unique
    // (exactly the MFC NextUntitled algorithm).
    int next_number = 0;
    for (const auto &s : m_specs) {
        if (s.name.rfind("Untitled", 0) == 0)
            ++next_number;
    }
    std::string name;
    do {
        name = "Untitled " + std::to_string(++next_number);
    } while (indexByName(name) != -1);
    return name;
}

void AccessSpecLibrary::smartName(int index)
{
    if (index < 0 || index >= count())
        return;
    AccessSpec &spec = m_specs[index];

    // Smart naming is only possible for single-line specs (MFC checks
    // access[1].of_size == IOERROR); otherwise the existing name stays.
    if (spec.lines.size() != 1)
        return;
    const AccessSpecLine &l = spec.lines[0];

    // Shared formatting (also used by the MFC SmartName on the wire model).
    std::string name = smartNameText(l.sizeBytes, 100 - l.seqPercent, l.readPercent);

    // Ensure uniqueness with a " 2", " 3", ... suffix (a spec keeping its own
    // name is fine - skip itself in the collision check).
    const std::string base = name;
    int spec_number = 1;
    int hit;
    while ((hit = indexByName(name)) != -1 && hit != index)
        name = base + " " + std::to_string(++spec_number);

    spec.name = name;
}

} // namespace iocore
