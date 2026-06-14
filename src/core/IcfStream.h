#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

// Platform-neutral port of the MFC ICF_ifstream (ICF_ifstream.h/.cpp) - the
// low-level primitives every ICF section loader is built on. The MFC behavior is
// CANONICAL (see ICF_PARSER_UNIFICATION_PLAN.md): line length limit, trimming,
// case-insensitive prefix matching, the dual version-encoding scheme (including
// its dead 3.26.97 special case), and the exact error-message texts (the MFC
// code shows them in dialogs; here they are collected via errors()/lastError()
// so a front-end adapter can display them verbatim).

namespace iocore {

class IcfStream {
public:
    // MFC MAX_ICF_LINE: lines are read with getline(buf, 200); a longer line
    // truncates to 199 chars and puts the stream in a failed state.
    static const int MaxLine = 200;

    // getVersion() error result (the MFC code returns -1 in a DWORDLONG).
    static const uint64_t BadVersion = ~0ULL;

    // Binary mode: byte-exact tellg/seekg (getPair's empty-section push-back).
    // MFC opens text-mode, which only seeks correctly on CRLF files - the only
    // kind the Windows GUI writes; trimming handles the '\r', so behavior for
    // every CRLF file is identical to MFC, and LF-only files work too (in MFC
    // text mode those could mis-seek - a glitch, not a behavior to preserve).
    explicit IcfStream(const std::string &path) : m_file(path, std::ios::binary) {}

    bool isOpen() const { return m_file.is_open(); }
    bool eof()    const { return m_file.eof(); }
    // MFC callers test rdstate() directly ("if (infile.rdstate())").
    bool failed() const { return m_file.rdstate() != std::ios::goodbit; }
    int  peek()         { return m_file.peek(); }

    // Read the version line ("Version 1.1.0" / "Version 1998.10.08") and encode:
    //   major > 1900 (date):   major*10000 + middle*100 + minor
    //   else       (semver):   major*10000000000 + middle*100000 + minor
    // so semver always orders above every date version. The version==32697
    // special case (3.26.97 -> 19980105) is preserved verbatim even though the
    // current formulas make it unreachable from "3.26.97" itself (Phase-0
    // verified). Versions < 19980105 record an error but are still returned.
    uint64_t getVersion();

    // Skip lines until one *starts with* `identifier` (case-insensitive, both
    // sides trimmed). Consumes the matched line. False at EOF.
    bool skipTo(const std::string &identifier);

    // Next line, trimmed both ends. Lines >= MaxLine chars truncate and fail the
    // stream (MFC getline(buf,200) behavior). Empty string on closed file.
    std::string getNextLine();

    // Read a 'key comment line and its following value line. 'End*/'Version*
    // keys have no value. Extra comment lines before the value are skipped -
    // unless one is an 'End*/'Version* line (empty section), which is pushed
    // back for the next call and an empty value returned (success).
    bool getPair(std::string &key, std::string &value);

    // Raw stream access for the access-spec parsers, which read with
    // operator>> and getline-with-delimiter exactly like the MFC code.
    std::ifstream &raw() { return m_file; }

    // Collected MFC error texts (ErrorMessage equivalents), oldest first.
    const std::vector<std::string> &errors() const { return m_errors; }
    std::string lastError() const { return m_errors.empty() ? std::string() : m_errors.back(); }

    // ---- static field extractors (port of the ICF_ifstream statics) ---------
    // On failure they leave/restore `s` per MFC and write the exact MFC error
    // text to *err (when provided).
    static bool extractFirstInt(std::string &s, int &n, std::string *err = nullptr);
    static bool extractFirstUInt64(std::string &s, uint64_t &n, std::string *err = nullptr);
    static bool extractFirstIntVersion(std::string &s, int &n, std::string *err = nullptr);
    // First token of ICF "token characters" (no comma/pipe; spaces only when
    // requested); consumes it from `s` and trims the remainder.
    static std::string extractFirstToken(std::string &s, bool spaces = false);

private:
    void addError(const std::string &msg) { m_errors.push_back(msg); }

    std::ifstream            m_file;
    std::vector<std::string> m_errors;
};

} // namespace iocore
