// tst_wirestructs - locks the on-the-wire layout of Iometer's protocol structs.
//
// Two jobs:
//   1. Proves the manager-core / Qt build can include the PROJECT-WIDE canonical
//      wire structs (IOAccess.h / IOTest.h, via IOCommon.h) with NO MFC.
//      IOMTR_NO_MFC makes IOCommon pull <windows.h> instead of StdAfx/afxwin.h.
//   2. Pins every struct's size and every field's byte offset. Dynamo and the
//      managers exchange these structs as raw bytes, so the layout MUST be
//      identical on every platform/compiler. The static_asserts below make a
//      divergent build (different int width, packing, enum size, etc.) FAIL TO
//      COMPILE with a named message - the cheapest possible place to catch it.
//
// Reference values were captured on Windows x64 (MSVC, FORCE_STRUCT_ALIGN /
// pack(1)), which is the canonical wire format every other platform must match.
// All structs are 1-byte packed, so each offset is just the running sum of the
// preceding fields - no alignment padding to reason about.

#include <QtTest>
#include <cstddef>
#include "IOAccess.h"   // Access_Spec
#include "IOTest.h"     // Test_Spec / Disk_Spec / TCP_Spec / VI_Spec / Target_Spec

// ── 1. Fundamental wire types (everything else is built from these) ──────────
static_assert(sizeof(char)         == 1, "char must be 1 byte");
static_assert(sizeof(int)          == 4, "int must be 4 bytes on the wire");
static_assert(sizeof(unsigned int) == 4, "unsigned int must be 4 bytes on the wire");
static_assert(sizeof(DWORD)        == 4, "DWORD must be a 32-bit unsigned");
static_assert(sizeof(DWORDLONG)    == 8, "DWORDLONG must be a 64-bit unsigned");
static_assert(sizeof(BOOL)         == 4, "BOOL must marshal as a 4-byte int");
static_assert(sizeof(TargetType)   == 4, "TargetType enum must be 4 bytes");
static_assert(sizeof(VIP_NET_ADDRESS) == 8, "VIP_NET_ADDRESS must be 8 bytes");

// ── 2. Access_Spec (32 bytes): 5x int + 3x DWORD ────────────────────────────
static_assert(sizeof(Access_Spec) == 32, "Access_Spec wire size changed");
static_assert(offsetof(Access_Spec, of_size) == 0,  "Access_Spec::of_size moved");
static_assert(offsetof(Access_Spec, reads)   == 4,  "Access_Spec::reads moved");
static_assert(offsetof(Access_Spec, random)  == 8,  "Access_Spec::random moved");
static_assert(offsetof(Access_Spec, delay)   == 12, "Access_Spec::delay moved");
static_assert(offsetof(Access_Spec, burst)   == 16, "Access_Spec::burst moved");
static_assert(offsetof(Access_Spec, align)   == 20, "Access_Spec::align moved");
static_assert(offsetof(Access_Spec, reply)   == 24, "Access_Spec::reply moved");
static_assert(offsetof(Access_Spec, size)    == 28, "Access_Spec::size moved");

// ── 3. Test_Spec (3332 bytes): name[128] + int + Access_Spec[100] ───────────
static_assert(sizeof(Test_Spec) == 3332, "Test_Spec wire size changed");
static_assert(offsetof(Test_Spec, name)               == 0,   "Test_Spec::name moved");
static_assert(offsetof(Test_Spec, default_assignment) == 128, "Test_Spec::default_assignment moved");
static_assert(offsetof(Test_Spec, access)             == 132, "Test_Spec::access moved");

// ── 4. Disk_Spec (24 bytes) ─────────────────────────────────────────────────
static_assert(sizeof(Disk_Spec) == 24, "Disk_Spec wire size changed");
static_assert(offsetof(Disk_Spec, ready)          == 0,  "Disk_Spec::ready moved");
static_assert(offsetof(Disk_Spec, sector_size)    == 4,  "Disk_Spec::sector_size moved");
static_assert(offsetof(Disk_Spec, maximum_size)   == 8,  "Disk_Spec::maximum_size moved");
static_assert(offsetof(Disk_Spec, starting_sector) == 16, "Disk_Spec::starting_sector moved");

// ── 5. TCP_Spec (88 bytes): uint + char[80] + uint ──────────────────────────
static_assert(sizeof(TCP_Spec) == 88, "TCP_Spec wire size changed");
static_assert(offsetof(TCP_Spec, local_port)     == 0,  "TCP_Spec::local_port moved");
static_assert(offsetof(TCP_Spec, remote_address) == 4,  "TCP_Spec::remote_address moved");
static_assert(offsetof(TCP_Spec, remote_port)    == 84, "TCP_Spec::remote_port moved");

// ── 6. VI_Spec (146 bytes) ──────────────────────────────────────────────────
static_assert(sizeof(VI_Spec) == 146, "VI_Spec wire size changed");
static_assert(offsetof(VI_Spec, local_address)     == 0,   "VI_Spec::local_address moved");
static_assert(offsetof(VI_Spec, remote_nic_name)   == 27,  "VI_Spec::remote_nic_name moved");
static_assert(offsetof(VI_Spec, remote_address)    == 107, "VI_Spec::remote_address moved");
static_assert(offsetof(VI_Spec, max_transfer_size) == 134, "VI_Spec::max_transfer_size moved");
static_assert(offsetof(VI_Spec, max_connections)   == 138, "VI_Spec::max_connections moved");
static_assert(offsetof(VI_Spec, outstanding_ios)   == 142, "VI_Spec::outstanding_ios moved");

// ── 7. Target_Spec (266 bytes): the union is VI_Spec-sized (largest member) ──
static_assert(sizeof(Target_Spec) == 266, "Target_Spec wire size changed");
static_assert(offsetof(Target_Spec, name)                == 0,   "Target_Spec::name moved");
static_assert(offsetof(Target_Spec, test_connection_rate) == 80,  "Target_Spec::test_connection_rate moved");
static_assert(offsetof(Target_Spec, type)                == 84,  "Target_Spec::type moved");
static_assert(offsetof(Target_Spec, disk_info)           == 88,  "Target_Spec union moved");
static_assert(offsetof(Target_Spec, queue_depth)         == 234, "Target_Spec::queue_depth moved (union size changed?)");
static_assert(offsetof(Target_Spec, DataPattern)         == 238, "Target_Spec::DataPattern moved");
static_assert(offsetof(Target_Spec, trans_per_conn)      == 242, "Target_Spec::trans_per_conn moved");
static_assert(offsetof(Target_Spec, random)              == 246, "Target_Spec::random moved");
static_assert(offsetof(Target_Spec, use_fixed_seed)      == 254, "Target_Spec::use_fixed_seed moved");
static_assert(offsetof(Target_Spec, fixed_seed_value)    == 258, "Target_Spec::fixed_seed_value moved");

// ── Runtime mirror of the above ─────────────────────────────────────────────
// The static_asserts are the real guard (compile-time). These slots give the
// test suite a visible pass and, on a divergent platform that somehow compiles,
// print expected-vs-actual so the drift is easy to read.
class TestWireStructs : public QObject {
    Q_OBJECT

private slots:
    void fundamentalTypeSizes();
    void accessSpecLayout();
    void testSpecLayout();
    void diskSpecLayout();
    void tcpSpecLayout();
    void viSpecLayout();
    void targetSpecLayout();
};

// Small helper so a failure message names the field.
#define CHECK_OFFSET(S, F, E) \
    QVERIFY2(offsetof(S, F) == (E), #S "::" #F " offset != " #E)
#define CHECK_SIZE(T, E) \
    QVERIFY2(sizeof(T) == (E), "sizeof(" #T ") != " #E)

void TestWireStructs::fundamentalTypeSizes() {
    CHECK_SIZE(DWORD, 4u);
    CHECK_SIZE(DWORDLONG, 8u);
    CHECK_SIZE(BOOL, 4u);
    CHECK_SIZE(TargetType, 4u);
    CHECK_SIZE(VIP_NET_ADDRESS, 8u);
}

void TestWireStructs::accessSpecLayout() {
    CHECK_SIZE(Access_Spec, 32u);
    CHECK_OFFSET(Access_Spec, of_size, 0);
    CHECK_OFFSET(Access_Spec, reads,   4);
    CHECK_OFFSET(Access_Spec, random,  8);
    CHECK_OFFSET(Access_Spec, delay,   12);
    CHECK_OFFSET(Access_Spec, burst,   16);
    CHECK_OFFSET(Access_Spec, align,   20);
    CHECK_OFFSET(Access_Spec, reply,   24);
    CHECK_OFFSET(Access_Spec, size,    28);
}

void TestWireStructs::testSpecLayout() {
    CHECK_SIZE(Test_Spec, 3332u);
    CHECK_OFFSET(Test_Spec, name,               0);
    CHECK_OFFSET(Test_Spec, default_assignment, 128);
    CHECK_OFFSET(Test_Spec, access,             132);
}

void TestWireStructs::diskSpecLayout() {
    CHECK_SIZE(Disk_Spec, 24u);
    CHECK_OFFSET(Disk_Spec, ready,           0);
    CHECK_OFFSET(Disk_Spec, sector_size,     4);
    CHECK_OFFSET(Disk_Spec, maximum_size,    8);
    CHECK_OFFSET(Disk_Spec, starting_sector, 16);
}

void TestWireStructs::tcpSpecLayout() {
    CHECK_SIZE(TCP_Spec, 88u);
    CHECK_OFFSET(TCP_Spec, local_port,     0);
    CHECK_OFFSET(TCP_Spec, remote_address, 4);
    CHECK_OFFSET(TCP_Spec, remote_port,    84);
}

void TestWireStructs::viSpecLayout() {
    CHECK_SIZE(VI_Spec, 146u);
    CHECK_OFFSET(VI_Spec, local_address,     0);
    CHECK_OFFSET(VI_Spec, remote_nic_name,   27);
    CHECK_OFFSET(VI_Spec, remote_address,    107);
    CHECK_OFFSET(VI_Spec, max_transfer_size, 134);
    CHECK_OFFSET(VI_Spec, max_connections,   138);
    CHECK_OFFSET(VI_Spec, outstanding_ios,   142);
}

void TestWireStructs::targetSpecLayout() {
    CHECK_SIZE(Target_Spec, 266u);
    CHECK_OFFSET(Target_Spec, name,                 0);
    CHECK_OFFSET(Target_Spec, test_connection_rate, 80);
    CHECK_OFFSET(Target_Spec, type,                 84);
    CHECK_OFFSET(Target_Spec, disk_info,            88);
    CHECK_OFFSET(Target_Spec, queue_depth,          234);
    CHECK_OFFSET(Target_Spec, DataPattern,          238);
    CHECK_OFFSET(Target_Spec, trans_per_conn,       242);
    CHECK_OFFSET(Target_Spec, random,               246);
    CHECK_OFFSET(Target_Spec, use_fixed_seed,       254);
    CHECK_OFFSET(Target_Spec, fixed_seed_value,     258);
}

QTEST_APPLESS_MAIN(TestWireStructs)
#include "tst_wirestructs.moc"
