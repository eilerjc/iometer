#pragma once

#include <string>
#include <vector>
#include "IometerTypes.h"

// Platform-neutral access-specification helpers shared by both GUIs:
//   - naming/formatting (was Qt formatSizeCompact / accessSpecDisplayLabel and
//     MFC AccessSpecList::SmartName),
//   - the built-in/default access specifications (was Qt builtinAccessSpecs() and
//     MFC AccessSpecList::InsertDefaultSpecs).
// Operate on the friendly core model (AccessSpec / AccessSpecLine); no GUI types.

namespace iocore {

// Format a size in bytes as the largest unit that fits: "X MiB" / "Y KiB" / "Z B".
std::string formatSize(int bytes);

// Display label for an access spec. A single-line spec renders as
// "64 KiB; 100% Read; 0% random"; a multi-line (or empty) spec renders as its name.
std::string accessSpecLabel(const AccessSpec &spec);

// The "smart name" text for a single I/O pattern (MFC AccessSpecList::SmartName's
// formatting): "<size> <random part> <read part>", e.g. "4 KiB random reads",
// "512 B sequential 50% reads", "8 KiB 30% random writes". Size rule: 512 ->
// "512 B", 1 MiB -> "1 MiB", else "N KiB". Uniqueness suffixes are the caller's
// concern (they need the spec list).
std::string smartNameText(int sizeBytes, int randomPct, int readPct);

// The built-in access specifications: Idle, Default (2 KiB, 67% read, random -
// the canonical MFC InitAccessSpecLine values), the size x read-mix matrix, and
// the combined "All in one" spec.
std::vector<AccessSpec> defaultAccessSpecs();

// Result of validating an access specification for the editor's OK/Save action.
struct SpecValidation {
    bool        ok;
    std::string error;   // exact MFC ErrorMessage text when !ok; empty when ok
};

// Validate an access spec the way the editor's OK does (port of MFC
// CAccessDialog::CheckAccess), in the same order with the same messages:
//   - every line's size must be > 0
//   - the lines' "% of access" (ofSize) must sum to exactly 100
//   - the name (already trimmed by the caller) must be non-empty
//   - the name must not contain a comma (commas break the CSV results file)
//   - the name must be unique vs `otherNames` (case-insensitive, like MFC
//     strcasecmp) - pass every OTHER spec's name (exclude the one being edited)
// MFC is the canonical oracle; the Qt editor previously did NONE of this.
SpecValidation validateAccessSpec(const AccessSpec &spec,
                                  const std::vector<std::string> &otherNames);

// Fill one wire access-spec row from the friendly AccessSpecLine model. Templated
// on the wire struct so it works for ANY type with the canonical fields
// (of_size/reads/random/delay/burst/reply/size/align) - the canonical Access_Spec
// (IOAccess.h) or a GUI's byte-compatible equivalent (e.g. Qt's DyAccessSpec).
// The friendly->wire semantics live here so both GUIs share one mapping:
//   random  = 100 - seqPercent
//   size    = sizeBytes (0 -> 65536 default)
//   align   = sizeBytes-if-(-1) / 512-if-0 / explicit-if->0
template <typename WireAccess>
void fillWireAccess(WireAccess &a, const AccessSpecLine &line)
{
    a.of_size = line.ofSize;
    a.reads   = line.readPercent;
    a.random  = 100 - line.seqPercent;
    a.delay   = static_cast<decltype(a.delay)>(line.delayMs);
    a.burst   = line.burstLength;
    a.reply   = static_cast<decltype(a.reply)>(line.replyBytes);
    a.size    = static_cast<decltype(a.size)>(line.sizeBytes > 0 ? line.sizeBytes : 65536);
    if      (line.alignBytes < 0)  a.align = a.size;                                  // request-size
    else if (line.alignBytes == 0) a.align = static_cast<decltype(a.align)>(512);      // sector
    else                           a.align = static_cast<decltype(a.align)>(line.alignBytes);
}

} // namespace iocore
