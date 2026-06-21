#pragma once

// Shared Megabytes/Kilobytes/Bytes <-> bytes conversion for the access-spec
// editors (transfer size, alignment, reply size). Both GUIs present the value
// as three spinners (MiB / KiB / B); this is the single source for composing and
// decomposing them, matching IOCommon's KILOBYTE_BIN (1024) / MEGABYTE_BIN
// (1048576). Was duplicated in MFC CAccessDialog::Get/SetMKB* and Qt
// read/setSizeSpinners.

namespace iocore {

struct Mkb {
    int megabytes;
    int kilobytes;
    int bytes;
};

// Compose a MiB/KiB/B triple into a total byte count.
inline int mkbToBytes(int megabytes, int kilobytes, int bytes) {
    return megabytes * 1048576 + kilobytes * 1024 + bytes;
}

// Decompose a total byte count into the MiB/KiB/B spinner values (each the
// remainder at its level, exactly as both editors display them).
inline Mkb bytesToMkb(int totalBytes) {
    Mkb m;
    m.megabytes = totalBytes / 1048576;
    m.kilobytes = (totalBytes % 1048576) / 1024;
    m.bytes     = totalBytes % 1024;
    return m;
}

} // namespace iocore
