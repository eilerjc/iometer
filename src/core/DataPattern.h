#pragma once

// Canonical catalog of the write-buffer data patterns. This is the single place
// that defines WHICH patterns exist and their display names; both GUIs build
// their "Data pattern" combo from it, and Dynamo's fill paths switch on the same
// ids. To add a new pattern:
//   1. add an id + name here (dataPatternCount()/dataPatternValid() pick it up,
//      and both GUI combos list it automatically),
//   2. add its fill behavior in Dynamo's write path (Grunt::Do_IOs) and, if it
//      needs prepare-time setup, in TargetDisk::Prepare,
//   3. (nothing else: the value flows through the wire Target_Spec.DataPattern
//      and the ICF as a plain integer).
//
// The integer ids MUST match the DATA_PATTERN_* wire constants in IOCommon.h
// (Target_Spec.DataPattern / QtDyProto data_pattern). They are intentionally a
// small, stable set; keep the two in sync when adding a pattern.

namespace iocore {

enum DataPatternId {
    DataPatternRepeatingBytes = 0,	// each write: memset the buffer with a fresh random byte
    DataPatternPseudoRandom   = 1,	// buffer filled with random bytes once, at prepare
    DataPatternFullRandom     = 2,	// each write: a random aligned window of a random buffer

    DataPatternCount			// keep last - number of patterns
};

inline int dataPatternCount()
{
    return DataPatternCount;
}

inline bool dataPatternValid(int id)
{
    return id >= 0 && id < DataPatternCount;
}

// Safe fallback when an unknown id is seen (e.g. a newer controller driving an
// older Dynamo). Repeating-bytes is the historical default.
inline int dataPatternDefault()
{
    return DataPatternRepeatingBytes;
}

inline const char *dataPatternName(int id)
{
    switch (id) {
    case DataPatternRepeatingBytes:
        return "Repeating bytes";
    case DataPatternPseudoRandom:
        return "Pseudo-random";
    case DataPatternFullRandom:
        return "Full random";
    default:
        return dataPatternName(dataPatternDefault());
    }
}

} // namespace iocore
