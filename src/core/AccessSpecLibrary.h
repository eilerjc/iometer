#pragma once

#include <string>
#include <vector>
#include "IometerTypes.h"

// Platform-neutral global access-specification list management, shared by both
// GUIs. Ported from the MFC AccessSpecList (the canonical spec - its semantics
// take precedence): "Untitled n" naming for new specs, "Copy of X (n)" unique
// naming for copies, case-insensitive name lookup, the default new-spec line
// (2 KiB, 67% read, 100% random), and SmartName ("4 KiB random reads" style).
//
// Index-based (the friendly AccessSpec model has value semantics). Worker
// references to deleted specs are the front-end's concern (MFC clears them via
// its ManagerList; Qt via its engine) - removeAt only edits the list.

namespace iocore {

class AccessSpecLibrary {
public:
    // The MFC default new-spec line (AccessSpecList::InitAccessSpecLine):
    // 2048 B, 100% of size, 67% read, 100% random, burst 1, align = request size.
    static AccessSpecLine defaultLine();

    // Starts empty. loadDefaults() seeds Idle + the built-in catalog specs
    // (what the MFC constructor does via InsertIdleSpec/InsertDefaultSpecs).
    AccessSpecLibrary() = default;
    void loadDefaults();

    // Wholesale replace/read (e.g. ICF load, or syncing with a GUI's list).
    void setSpecs(std::vector<AccessSpec> specs) { m_specs = std::move(specs); }
    const std::vector<AccessSpec> &specs() const { return m_specs; }

    int count() const { return static_cast<int>(m_specs.size()); }
    AccessSpec       &at(int index)       { return m_specs[index]; }
    const AccessSpec &at(int index) const { return m_specs[index]; }

    // Create a new spec ("Untitled n", default line, not default-assigned).
    // Returns its index.
    int newSpec();

    // Duplicate the spec at `index` as "Copy of <name> (n)" (n = first unique).
    // Returns the new spec's index, or -1 if `index` is invalid.
    int copySpec(int index);

    // Remove the spec at `index`. Returns false if out of range.
    bool removeAt(int index);

    // Case-insensitive name lookup (MFC RefByName). -1 if not found.
    int indexByName(const std::string &name) const;

    // The next free "Untitled n" name (MFC NextUntitled).
    std::string nextUntitledName() const;

    // Assign the spec a descriptive name from its single line (MFC SmartName):
    // "<size> <random part> <read part>", e.g. "4 KiB random reads",
    // "512 B sequential 50% reads", unique-suffixed with " 2", " 3", ... .
    // Multi-line specs keep their existing name.
    void smartName(int index);

private:
    std::vector<AccessSpec> m_specs;
};

} // namespace iocore
