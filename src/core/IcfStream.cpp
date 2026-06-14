#include "IcfStream.h"
#include <cstdlib>
#include <cctype>

namespace iocore {

// ---- CString-semantics helpers ------------------------------------------------

static const char *WHITESPACE = " \t\r\n";

static void trimBoth(std::string &s)
{
    const std::size_t b = s.find_first_not_of(WHITESPACE);
    if (b == std::string::npos) { s.clear(); return; }
    const std::size_t e = s.find_last_not_of(WHITESPACE);
    s = s.substr(b, e - b + 1);
}

static void trimLeft(std::string &s)
{
    const std::size_t b = s.find_first_not_of(WHITESPACE);
    s = (b == std::string::npos) ? std::string() : s.substr(b);
}

// CString::CompareNoCase()==0 on the first `len` chars (Left(len) compare).
static bool startsWithNoCase(const std::string &s, const char *prefix)
{
    for (std::size_t i = 0; prefix[i]; ++i) {
        if (i >= s.size())
            return false;
        if (std::tolower(static_cast<unsigned char>(s[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

// CString::SpanIncluding(set): longest prefix consisting of chars in `set`.
static std::string spanIncluding(const std::string &s, const std::string &set)
{
    std::size_t n = 0;
    while (n < s.size() && set.find(s[n]) != std::string::npos)
        ++n;
    return s.substr(0, n);
}

// ---- line primitives ------------------------------------------------------------

std::string IcfStream::getNextLine()
{
    if (!m_file.is_open()) {
        // This should never happen.  This indicates an Iometer bug.
        addError("A call was made to ICF_ifstream::GetNextLine() "
                 "with a closed file!  Please report this as an Iometer bug.");
        return std::string();
    }

    std::string line;
    std::getline(m_file, line);
    // MFC reads getline(buf, MAX_ICF_LINE): a longer line truncates to 199
    // chars and fails the stream (which aborts the parse in every caller).
    if (line.size() >= static_cast<std::size_t>(MaxLine)) {
        line = line.substr(0, MaxLine - 1);
        m_file.setstate(std::ios::failbit);
    }

    trimBoth(line);
    return line;
}

bool IcfStream::skipTo(const std::string &identifier)
{
    std::string id = identifier;
    trimBoth(id);

    std::string curline;
    do {
        if (m_file.eof())
            return false;
        curline = getNextLine();   // already trimmed
    } while (!startsWithNoCase(curline, id.c_str()));
    // Loop until the identifier is matched.  (Ignore the rest of the line.)

    return true;
}

bool IcfStream::getPair(std::string &key, std::string &value)
{
    if (!m_file.is_open()) {
        // This should never happen.  This indicates an Iometer bug.
        addError("A call was made to ICF_ifstream::GetPair() "
                 "with a closed file!  Please report this as an Iometer bug.");
        return false;
    }
    // If EOF, let the caller report an error.
    if (failed())
        return false;

    // If first line isn't a comment, let the caller report an error.
    if (peek() != '\'')
        return false;

    key = getNextLine();

    // 'End and 'Version tags have no data on the following lines.
    if (startsWithNoCase(key, "'End") || startsWithNoCase(key, "'Version")) {
        value.clear();
        return true;
    }

    // Skip any extra comment lines before the data.
    while (peek() == '\'') {
        const std::streampos placeholder = m_file.tellg();
        const std::string tempstring = getNextLine();

        if (startsWithNoCase(tempstring, "'End") || startsWithNoCase(tempstring, "'Version")) {
            // Empty section (e.g. 'Worker immediately followed by 'End worker):
            // the 'End... header must be returned by the NEXT GetPair call.
            m_file.seekg(placeholder);   // back up the file pointer
            value.clear();
            return true;                 // NOT an error condition
        }
    }

    // If EOF, let the caller report an error.
    if (failed()) {
        value.clear();
        return false;
    }

    // Get the data corresponding to the above key.
    value = getNextLine();
    return true;
}

// ---- version ---------------------------------------------------------------------

uint64_t IcfStream::getVersion()
{
    std::string version_string = getNextLine();
    if (version_string.empty()) {
        addError("File is improperly formatted.  Empty line found "
                 "where file version information was expected.");
        return BadVersion;
    }

    int major = 0, middle = 0, minor = 0;
    std::string err;
    if (!extractFirstIntVersion(version_string, major, &err)
        || !extractFirstIntVersion(version_string, middle, &err)
        || !extractFirstIntVersion(version_string, minor, &err)) {
        if (!err.empty())
            addError(err);   // the extractor's own MFC dialog text
        addError("File is improperly formatted.  Error retrieving file version information.");
        return BadVersion;
    }

    // Dual scheme (see header): date versions vs the 1.1.0+ numbering scheme.
    uint64_t version;
    if (major > 1900)
        version = (uint64_t)major * 10000 + (uint64_t)middle * 100 + minor;
    else
        version = (uint64_t)major * 10000000000ULL + (uint64_t)middle * 100000 + minor;

    // Special case test for ancient version "3.26.97" OLTP.txt file distributed
    // with 1998.01.05. (Dead code under the current formulas - kept verbatim.)
    if (version == 32697)
        version = 19980105;

    if (version < 19980105) {
        // The MFC code shows this error but still proceeds with the version.
        addError("Error restoring file.  "
                 "Version number earlier than 1998.01.05 or incorrectly formatted.");
    }

    return version;
}

// ---- static extractors -------------------------------------------------------------

// Shared body of ExtractFirstInt / ExtractFirstUInt64 (identical except for the
// final conversion).
static bool extractSignedDigits(std::string &s, std::string &digits, std::string *err)
{
    const std::string backup = s;

    const std::size_t pos = s.find_first_of("-1234567890");
    if (pos == std::string::npos) {
        if (err) *err = "File is improperly formatted.  Expected an integer value.";
        return false;
    }
    // Cleave off everything before the first number.
    s = s.substr(pos);

    digits = spanIncluding(s, "-1234567890");
    s = s.substr(digits.size());

    // If there are any negative signs after the first character, fail.
    if (digits.find('-', 1) != std::string::npos) {
        if (err) *err = "File is improperly formatted.  An integer value "
                        "has a negative sign in the middle or on the end of it.";
        s = backup;   // restore string's old value
        return false;
    }

    // Eat whitespace, one trailing comma, and any space following it.
    trimLeft(s);
    if (!s.empty() && s[0] == ',')
        s = s.substr(1);
    trimLeft(s);
    return true;
}

bool IcfStream::extractFirstInt(std::string &s, int &n, std::string *err)
{
    n = 0;
    std::string digits;
    if (!extractSignedDigits(s, digits, err))
        return false;
    n = std::atoi(digits.c_str());
    return true;
}

bool IcfStream::extractFirstUInt64(std::string &s, uint64_t &n, std::string *err)
{
    n = 0;
    std::string digits;
    if (!extractSignedDigits(s, digits, err))
        return false;
    n = std::strtoull(digits.c_str(), nullptr, 10);
    return true;
}

bool IcfStream::extractFirstIntVersion(std::string &s, int &n, std::string *err)
{
    n = 0;

    const std::size_t pos = s.find_first_of("1234567890");
    if (pos == std::string::npos) {
        if (err) *err = "File is improperly formatted.  Expected an integer value.";
        return false;
    }
    // Cleave off everything before the first number.
    s = s.substr(pos);

    const std::string digits = spanIncluding(s, "1234567890");
    s = s.substr(digits.size());

    n = std::atoi(digits.c_str());

    // Eat whitespace, one trailing comma, and any space following it.
    trimLeft(s);
    if (!s.empty() && s[0] == ',')
        s = s.substr(1);
    trimLeft(s);
    return true;
}

std::string IcfStream::extractFirstToken(std::string &s, bool spaces)
{
    std::string token_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz" "1234567890" "`~!@#$%^&*()-_=+[]{}\\:;\"'./<>?";
    if (spaces)
        token_chars += " ";

    const std::size_t pos = s.find_first_of(token_chars);
    if (pos == std::string::npos)
        return std::string();

    s = s.substr(pos);
    const std::string substring = spanIncluding(s, token_chars);
    s = s.substr(substring.size());
    trimBoth(s);

    return substring;
}

} // namespace iocore
