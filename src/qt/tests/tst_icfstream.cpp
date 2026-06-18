// tst_icfstream - the iocore::IcfStream port of MFC's ICF_ifstream primitives.
// MFC behavior is canonical (docs/dev/ICF_PARSER_UNIFICATION_PLAN.md, Phase 1); the
// version-table expectations were verified empirically in Phase 0.
#include <QtTest>
#include <QTemporaryDir>
#include <fstream>
#include "IcfStream.h"

using iocore::IcfStream;

class TestIcfStream : public QObject {
    Q_OBJECT

    QTemporaryDir m_dir;
    int           m_n = 0;

    // Write `content` to a fresh temp file; return its path.
    std::string makeFile(const std::string &content) {
        const std::string p = QDir(m_dir.path())
            .filePath(QString("icf_%1.txt").arg(m_n++)).toStdString();
        std::ofstream f(p, std::ios::binary);
        f << content;
        return p;
    }

private slots:
    // version table
    void version_semver110();
    void version_dateScheme();
    void version_32697_isDeadCode();
    void version_explicitSpecialCase();
    void version_preHistoric_warnsButContinues();
    void version_emptyLine_fails();
    void version_garbage_fails();
    // primitives
    void skipTo_caseInsensitivePrefix();
    void skipTo_missing_returnsFalse();
    void getNextLine_trims();
    void getNextLine_longLineTruncatesAndFails();
    void getPair_keyAndValue();
    void getPair_endKeyHasNoValue();
    void getPair_emptySection_pushesBackEnd();
    void getPair_emptySection_crlf();
    void getPair_skipsExtraComments();
    // extractors
    void extractInt_basicAndList();
    void extractInt_negative();
    void extractInt_midNegative_failsAndRestores();
    void extractInt_noDigits_failsWithMfcText();
    void extractUInt64_large();
    void extractToken_stopsAtComma();
    void extractToken_withSpaces_specName();
    // error / edge branches
    void getPair_nonCommentFirstLine_returnsFalse();
    void extractToken_noTokenChars_returnsEmpty();
    void closedFile_primitivesFailGracefully();
};

void TestIcfStream::version_semver110() {
    IcfStream s(makeFile("Version 1.1.0 \n"));
    QCOMPARE(s.getVersion(), Q_UINT64_C(10000100000));
}

void TestIcfStream::version_dateScheme() {
    IcfStream s(makeFile("Version 1998.10.08 \n"));
    QCOMPARE(s.getVersion(), Q_UINT64_C(19981008));
}

void TestIcfStream::version_32697_isDeadCode() {
    // "3.26.97" goes through the semver branch -> 30002600097, so the
    // ==32697 OLTP special case never fires for it (Phase-0 verified).
    IcfStream s(makeFile("Version 3.26.97\n"));
    QCOMPARE(s.getVersion(), Q_UINT64_C(30002600097));
    QVERIFY(s.errors().empty());   // >= 19980105, no warning
}

void TestIcfStream::version_explicitSpecialCase() {
    // The only way the special case can still fire: a literal 0.0.32697.
    IcfStream s(makeFile("Version 0.0.32697\n"));
    QCOMPARE(s.getVersion(), Q_UINT64_C(19980105));
}

void TestIcfStream::version_preHistoric_warnsButContinues() {
    IcfStream s(makeFile("Version 1997.01.01\n"));
    QCOMPARE(s.getVersion(), Q_UINT64_C(19970101));   // still returned
    QCOMPARE(s.lastError(), std::string("Error restoring file.  "
        "Version number earlier than 1998.01.05 or incorrectly formatted."));
}

void TestIcfStream::version_emptyLine_fails() {
    IcfStream s(makeFile("\nVersion 1.1.0\n"));
    QCOMPARE(s.getVersion(), IcfStream::BadVersion);
    QCOMPARE(s.lastError(), std::string("File is improperly formatted.  "
        "Empty line found where file version information was expected."));
}

void TestIcfStream::version_garbage_fails() {
    IcfStream s(makeFile("Version x.y.z\n"));
    QCOMPARE(s.getVersion(), IcfStream::BadVersion);
    QCOMPARE(s.lastError(), std::string("File is improperly formatted.  "
        "Error retrieving file version information."));
}

void TestIcfStream::skipTo_caseInsensitivePrefix() {
    IcfStream s(makeFile("junk\n'access SPECIFICATIONS ====55\n\tnext\n"));
    QVERIFY(s.skipTo("'ACCESS SPECIFICATIONS"));
    QCOMPARE(s.getNextLine(), std::string("next"));   // matched line consumed
}

void TestIcfStream::skipTo_missing_returnsFalse() {
    IcfStream s(makeFile("a\nb\n"));
    QVERIFY(!s.skipTo("'NOPE"));
}

void TestIcfStream::getNextLine_trims() {
    IcfStream s(makeFile("\t  hello world \t\r\n"));
    QCOMPARE(s.getNextLine(), std::string("hello world"));
}

void TestIcfStream::getNextLine_longLineTruncatesAndFails() {
    // MFC getline(buf, 200): >=200-char lines truncate to 199 and fail the stream.
    const std::string longline(250, 'x');
    IcfStream s(makeFile(longline + "\nrest\n"));
    QCOMPARE((int)s.getNextLine().size(), IcfStream::MaxLine - 1);
    QVERIFY(s.failed());
}

void TestIcfStream::getPair_keyAndValue() {
    IcfStream s(makeFile("'Run Time\n\t0 0 3\n"));
    std::string k, v;
    QVERIFY(s.getPair(k, v));
    QCOMPARE(k, std::string("'Run Time"));
    QCOMPARE(v, std::string("0 0 3"));
}

void TestIcfStream::getPair_endKeyHasNoValue() {
    IcfStream s(makeFile("'End worker\n\tdata\n"));
    std::string k, v;
    QVERIFY(s.getPair(k, v));
    QCOMPARE(k, std::string("'End worker"));
    QVERIFY(v.empty());
    // The data line was NOT consumed.
    QCOMPARE(s.getNextLine(), std::string("data"));
}

void TestIcfStream::getPair_emptySection_pushesBackEnd() {
    // 'Worker immediately followed by 'End worker: first call returns the
    // 'Worker key with an empty value; the next call must see 'End worker.
    IcfStream s(makeFile("'Worker\n'End worker\n"));
    std::string k, v;
    QVERIFY(s.getPair(k, v));
    QCOMPARE(k, std::string("'Worker"));
    QVERIFY(v.empty());
    QVERIFY(s.getPair(k, v));
    QCOMPARE(k, std::string("'End worker"));
}

void TestIcfStream::getPair_emptySection_crlf() {
    // Same as above with CRLF line endings (what the Windows GUI writes).
    IcfStream s(makeFile("'Worker\r\n'End worker\r\n"));
    std::string k, v;
    QVERIFY(s.getPair(k, v));
    QCOMPARE(k, std::string("'Worker"));
    QVERIFY(v.empty());
    QVERIFY(s.getPair(k, v));
    QCOMPARE(k, std::string("'End worker"));
}

void TestIcfStream::getPair_skipsExtraComments() {
    IcfStream s(makeFile("'Run Time\n'\thours      minutes    seconds\n\t0          0          3\n"));
    std::string k, v;
    QVERIFY(s.getPair(k, v));
    QCOMPARE(k, std::string("'Run Time"));
    QCOMPARE(v, std::string("0          0          3"));
}

void TestIcfStream::extractInt_basicAndList() {
    std::string s = "12,34 56";
    int n = -1;
    QVERIFY(IcfStream::extractFirstInt(s, n));
    QCOMPARE(n, 12);
    QCOMPARE(s, std::string("34 56"));   // comma + space eaten
    QVERIFY(IcfStream::extractFirstInt(s, n));
    QCOMPARE(n, 34);
    QCOMPARE(s, std::string("56"));
}

void TestIcfStream::extractInt_negative() {
    std::string s = "-7";
    int n = 0;
    QVERIFY(IcfStream::extractFirstInt(s, n));
    QCOMPARE(n, -7);
}

void TestIcfStream::extractInt_midNegative_failsAndRestores() {
    std::string s = "12-3,x";
    int n = 99;
    std::string err;
    QVERIFY(!IcfStream::extractFirstInt(s, n, &err));
    QCOMPARE(s, std::string("12-3,x"));   // restored
    QCOMPARE(err, std::string("File is improperly formatted.  An integer value "
                              "has a negative sign in the middle or on the end of it."));
}

void TestIcfStream::extractInt_noDigits_failsWithMfcText() {
    std::string s = "abc";
    int n = 0;
    std::string err;
    QVERIFY(!IcfStream::extractFirstInt(s, n, &err));
    QCOMPARE(err, std::string("File is improperly formatted.  Expected an integer value."));
}

void TestIcfStream::extractUInt64_large() {
    std::string s = "16044720128";
    uint64_t n = 0;
    QVERIFY(IcfStream::extractFirstUInt64(s, n));
    QCOMPARE(n, Q_UINT64_C(16044720128));
}

void TestIcfStream::extractToken_stopsAtComma() {
    std::string s = "ENABLED,1";
    QCOMPARE(IcfStream::extractFirstToken(s), std::string("ENABLED"));
    QCOMPARE(s, std::string(",1"));   // comma is NOT a token char and stays
}

void TestIcfStream::extractToken_withSpaces_specName() {
    // How spec names parse: spaces allowed, the comma still terminates.
    std::string s = "64 KiB; 100% Read; 0% random,NONE";
    QCOMPARE(IcfStream::extractFirstToken(s, true),
             std::string("64 KiB; 100% Read; 0% random"));
    QCOMPARE(s, std::string(",NONE"));
}

void TestIcfStream::getPair_nonCommentFirstLine_returnsFalse() {
    // getPair requires the next line to be a comment ('...); a data line -> false.
    IcfStream s(makeFile("notacomment\n\tvalue\n"));
    std::string k, v;
    QVERIFY(!s.getPair(k, v));
}

void TestIcfStream::extractToken_noTokenChars_returnsEmpty() {
    // No token characters (only commas/spaces) -> empty token.
    std::string s = ",, ,";
    QCOMPARE(IcfStream::extractFirstToken(s), std::string());
}

void TestIcfStream::closedFile_primitivesFailGracefully() {
    // Constructing on a path that doesn't open leaves the stream closed; the
    // primitives must report the "closed file" Iometer-bug error, not crash.
    const std::string bad = QDir(m_dir.path()).filePath("does_not_exist.icf").toStdString();
    IcfStream s(bad);
    QVERIFY(s.getNextLine().empty());
    std::string k, v;
    QVERIFY(!s.getPair(k, v));
    QVERIFY(!s.errors().empty());
}

QTEST_APPLESS_MAIN(TestIcfStream)
#include "tst_icfstream.moc"
