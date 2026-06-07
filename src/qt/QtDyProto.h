// DyProto.h - Iometer/Dynamo TCP wire-protocol structures
//
// Structs match the Windows Dynamo build (msvs11) which defines:
//   FORCE_STRUCT_ALIGN → #pragma pack(push,1)  (no natural padding)
//   USE_NEW_DISCOVERY_MECHANISM → extra name fields in Target_Spec/Disk_Spec
//
// Sizes (verified against source math):
//   DyMsg               =          8 bytes
//   DyManagerInfo       =        350 bytes
//   DyRawResult         =        296 bytes
//   DyTargetSpec        =        434 bytes
//   DyWorkerResults     =    606,228 bytes
//   DyManagerResults    =      4,640 bytes
//   DyDataMessage       =    888,840 bytes   ← DATA_MESSAGE_SIZE
#pragma once
#include <cstdint>

// --- Constants ---------------------------------------------------------------

static constexpr uint16_t DY_PORT           = 1066;
static constexpr int32_t  DY_VERSION        = (1 << 24) | (1 << 8);  // 1.1.0 = 0x01000100

static constexpr int32_t  DY_REPLY_FILTER   = 0x10000000;
static constexpr int32_t  DY_NO_REPLY       = 0x01000000;

static constexpr int32_t  DY_LOGIN          = DY_REPLY_FILTER + 1;
static constexpr int32_t  DY_ADD_WORKERS    = DY_REPLY_FILTER + 2;
static constexpr int32_t  DY_REPORT_TARGETS = DY_REPLY_FILTER + 3;
static constexpr int32_t  DY_START          = DY_REPLY_FILTER + 4;
static constexpr int32_t  DY_BEGIN_IO       = DY_REPLY_FILTER + 5;
static constexpr int32_t  DY_RECORD_OFF     = DY_REPLY_FILTER + 6;
static constexpr int32_t  DY_STOP           = DY_REPLY_FILTER + 7;
static constexpr int32_t  DY_REPORT_RESULTS = DY_REPLY_FILTER + 8;
static constexpr int32_t  DY_REPORT_UPDATE  = DY_REPLY_FILTER + 9;
static constexpr int32_t  DY_STOP_PREPARE   = DY_REPLY_FILTER + 10;
static constexpr int32_t  DY_SET_TARGETS    = DY_REPLY_FILTER + 11;
static constexpr int32_t  DY_SET_ACCESS     = DY_REPLY_FILTER + 12;
static constexpr int32_t  DY_LOGIN_OK       = DY_REPLY_FILTER + 15;
static constexpr int32_t  DY_WRONG_VERSION  = DY_REPLY_FILTER + 16;

static constexpr int32_t  DY_READY          = DY_NO_REPLY + 1;
static constexpr int32_t  DY_RECORD_ON      = DY_NO_REPLY + 2;
static constexpr int32_t  DY_RESET          = DY_NO_REPLY + 3;
static constexpr int32_t  DY_EXIT           = DY_NO_REPLY + 4;

static constexpr int32_t  DY_ALL_WORKERS    = -4;
static constexpr int32_t  DY_MANAGER        = -8;

static constexpr uint32_t DY_COMPAT_MASK    = 0xFFFFFF00u;  // ignore sub-minor

// Source constants
static constexpr int DY_MAX_NAME            = 80;
static constexpr int DY_MAX_NETWORK_NAME    = 128;
static constexpr int DY_MAX_WORKER_NAME     = 128;
static constexpr int DY_MAX_TARGETS         = 2048;
static constexpr int DY_MAX_SNAPSHOTS       = 2;
static constexpr int DY_MAX_CPUS            = 64;
static constexpr int DY_MAX_INTERFACES      = 64;
static constexpr int DY_CPU_RESULTS         = 6;
static constexpr int DY_TCP_RESULTS         = 1;
static constexpr int DY_NI_RESULTS          = 3;
static constexpr int DY_LATENCY_BINS        = 21;
static constexpr int DY_MAX_ACCESS_SPECS    = 100;
static constexpr int DY_MAX_VERSION_LEN     = 80;

// Performance snapshots
static constexpr int DY_FIRST_SNAP         = 0;
static constexpr int DY_LAST_SNAP          = 1;

// --- #pragma pack(push,1) -- matches FORCE_STRUCT_ALIGN ---------------------
#pragma pack(push, 1)

// -- 8-byte control message ---------------------------------------------------
struct DyMsg {
    int32_t purpose;
    int32_t data;
};
static_assert(sizeof(DyMsg) == 8, "DyMsg size mismatch");

// -- Manager_Info (sent by Dynamo at login) -----------------------------------
struct DyManagerInfo {
    char     version[DY_MAX_VERSION_LEN];       // "1.1.0"
    char     names[2][DY_MAX_NETWORK_NAME];     // [0]=hostname [1]=main-port host
    uint16_t port_number;                        // main-port number
    int32_t  processors;
    double   timer_resolution;                   // ticks/second (QPC frequency)
};
// 80 + 256 + 2 + 4 + 8 = 350 bytes
static_assert(sizeof(DyManagerInfo) == 350, "DyManagerInfo size mismatch");

// -- Access_Spec --------------------------------------------------------------
struct DyAccessSpec {
    int32_t  of_size;    // percentage of total size for this entry
    int32_t  reads;      // read percentage
    int32_t  random;     // random percentage
    int32_t  delay;      // inter-burst delay ms
    int32_t  burst;      // burst length
    uint32_t align;      // transfer alignment in bytes
    uint32_t reply;      // reply size for network
    uint32_t size;       // transfer size in bytes
};
static_assert(sizeof(DyAccessSpec) == 32, "DyAccessSpec size mismatch");

// -- Test_Spec ----------------------------------------------------------------
struct DyTestSpec {
    char        name[DY_MAX_WORKER_NAME];
    int32_t     default_assignment;
    DyAccessSpec access[DY_MAX_ACCESS_SPECS];
};
// 128 + 4 + 100*32 = 3332 bytes
static_assert(sizeof(DyTestSpec) == 3332, "DyTestSpec size mismatch");

// -- Disk_Spec (USE_NEW_DISCOVERY_MECHANISM) ----------------------------------
struct DyDiskSpec {
    uint32_t disk_reserved[5];    // 20 bytes
    int32_t  has_partitions;
    int32_t  not_mounted;
    int32_t  ready;
    int32_t  sector_size;
    uint64_t maximum_size;
    uint64_t starting_sector;
};
// 20 + 4 + 4 + 4 + 4 + 8 + 8 = 52 bytes
static_assert(sizeof(DyDiskSpec) == 52, "DyDiskSpec size mismatch");

// Network TargetType values (mirror src/IOTest.h TargetType, shared with MFC).
static constexpr int32_t DY_TCP_SERVER_TYPE = 0x800C8000;  // GenericServer | TCP
static constexpr int32_t DY_TCP_CLIENT_TYPE = 0x800A8000;  // GenericClient | TCP

// -- TCP_Spec -----------------------------------------------------------------
struct DyTcpSpec {
    uint32_t local_port;
    char     remote_address[DY_MAX_NAME];
    uint32_t remote_port;
};
// 4 + 80 + 4 = 88 bytes
static_assert(sizeof(DyTcpSpec) == 88, "DyTcpSpec size mismatch");

// -- VI_Spec (Virtual Interface - legacy, rarely used) ------------------------
struct DyViNetAddr {
    uint16_t host_addr_len;
    uint16_t discrim_len;
    uint8_t  host_address[1];
    char     padding[3];          // 8 - 2*2 - 1 = 3
};
static_assert(sizeof(DyViNetAddr) == 8, "DyViNetAddr size mismatch");

static constexpr int DY_VI_ADDR_SIZE   = 16;
static constexpr int DY_VI_DISCR_SIZE  = 4;  // sizeof(int)

struct DyViSpec {
    DyViNetAddr local_address;                                    // 8
    char        local_address_fill[DY_VI_ADDR_SIZE + DY_VI_DISCR_SIZE - 1]; // 19
    char        remote_nic_name[DY_MAX_NAME];                    // 80
    DyViNetAddr remote_address;                                   // 8
    char        remote_address_fill[DY_VI_ADDR_SIZE + DY_VI_DISCR_SIZE - 1]; // 19
    int32_t     max_transfer_size;
    int32_t     max_connections;
    int32_t     outstanding_ios;
};
// 8 + 19 + 80 + 8 + 19 + 4 + 4 + 4 = 146 bytes
static_assert(sizeof(DyViSpec) == 146, "DyViSpec size mismatch");

// -- Target_Spec (USE_NEW_DISCOVERY_MECHANISM) --------------------------------
struct DyTargetSpec {
    char      name[DY_MAX_NAME];          // display name
    char      actual_name[DY_MAX_NAME];   // real device path (USE_NEW_DISCOVERY)
    char      basic_name[DY_MAX_NAME];    // sorting name   (USE_NEW_DISCOVERY)
    int32_t   read_only;                  // (USE_NEW_DISCOVERY)
    int32_t   reserved;                   // (USE_NEW_DISCOVERY)
    int32_t   test_connection_rate;
    int32_t   type;                        // TargetType enum

    union {
        DyDiskSpec disk_info;   // 52
        DyTcpSpec  tcp_info;    // 88
        DyViSpec   vi_info;     // 146 ← largest
        char       _pad[146];  // ensure union = 146 bytes
    };

    int32_t  queue_depth;
    int32_t  data_pattern;
    int32_t  trans_per_conn;
    // NO char padding[4] - removed by FORCE_STRUCT_ALIGN
    uint64_t random;
    int32_t  use_fixed_seed;
    uint64_t fixed_seed_value;
};
// 80+80+80 + 4+4 + 4+4 + 146 + 4+4+4 + 8+4+8 = 434 bytes
static_assert(sizeof(DyTargetSpec) == 434, "DyTargetSpec size mismatch");

// -- TargetType constants -----------------------------------------------------
static constexpr int32_t DY_PHYSICAL_DISK  = static_cast<int32_t>(0x8C000000);
static constexpr int32_t DY_LOGICAL_DISK   = static_cast<int32_t>(0x8A000000);
static constexpr int32_t DY_TCP_SERVER     = static_cast<int32_t>(0x800C8000);
static constexpr int32_t DY_TCP_CLIENT     = static_cast<int32_t>(0x800A8000);

// -- Raw_Result ---------------------------------------------------------------
struct DyRawResult {
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint64_t read_count;
    uint64_t write_count;
    uint64_t transaction_count;
    uint64_t connection_count;
    uint32_t read_errors;
    uint32_t write_errors;
    uint64_t max_raw_read_latency;
    uint64_t read_latency_sum;
    uint64_t max_raw_write_latency;
    uint64_t write_latency_sum;
    uint64_t max_raw_transaction_latency;
    uint64_t max_raw_connection_latency;
    uint64_t transaction_latency_sum;
    uint64_t connection_latency_sum;
    int64_t  counter_time;
    int64_t  latency_bin[DY_LATENCY_BINS];
};
// 6*8 + 2*4 + 9*8 + 8 + 21*8 = 48+8+72+8+168 = 304... let me recount
// 6×uint64: 48  +  2×uint32: 8  +  8×uint64: 64  +  int64: 8  +  21×int64: 168
// = 48+8+64+8+168 = 296
static_assert(sizeof(DyRawResult) == 296, "DyRawResult size mismatch");

// -- Target_Results ----------------------------------------------------------
struct DyTargetResults {
    int32_t     count;
    // NO char pad[4] - removed by FORCE_STRUCT_ALIGN
    DyRawResult result[DY_MAX_TARGETS];
};
// 4 + 2048*296 = 4 + 606208 = 606212
static_assert(sizeof(DyTargetResults) == 606212, "DyTargetResults size mismatch");

// -- Worker_Results -----------------------------------------------------------
struct DyWorkerResults {
    uint64_t        time[DY_MAX_SNAPSHOTS];  // 16
    DyTargetResults target_results;          // 606212
};
// 16 + 606212 = 606228
static_assert(sizeof(DyWorkerResults) == 606228, "DyWorkerResults size mismatch");

// -- CPU_Results --------------------------------------------------------------
struct DyCpuResults {
    int32_t count;
    // NO char pad[4]
    double  CPU_utilization[DY_MAX_CPUS][DY_CPU_RESULTS];
};
// 4 + 64*6*8 = 4 + 3072 = 3076
static_assert(sizeof(DyCpuResults) == 3076, "DyCpuResults size mismatch");

// -- Net_Results --------------------------------------------------------------
struct DyNetResults {
    double  tcp_stats[DY_TCP_RESULTS];       // 8
    int32_t ni_count;                         // 4
    // NO char pad[4]
    double  ni_stats[DY_MAX_INTERFACES][DY_NI_RESULTS];
};
// 8 + 4 + 64*3*8 = 12 + 1536 = 1548
static_assert(sizeof(DyNetResults) == 1548, "DyNetResults size mismatch");

// -- Manager_Results ----------------------------------------------------------
struct DyManagerResults {
    int64_t     time_counter[DY_MAX_SNAPSHOTS];  // 16
    DyCpuResults cpu_results;                     // 3076
    DyNetResults net_results;                     // 1548
};
// 16 + 3076 + 1548 = 4640
static_assert(sizeof(DyManagerResults) == 4640, "DyManagerResults size mismatch");

// -- Message_Data union -------------------------------------------------------
// sizeof(DyTargetSpec) = 434; 2048 × 434 = 888,832 (dominant member)
union DyMessageData {
    DyManagerInfo    manager_info;                   // 350
    DyTargetSpec     targets[DY_MAX_TARGETS];        // 2048 × 434 = 888832 ← largest
    DyTestSpec       spec;                           // 3332
    DyManagerResults manager_results;                // 4640
    DyWorkerResults  worker_results;                 // 606228
    char             _raw[888832];
};
static_assert(sizeof(DyMessageData) == 888832, "DyMessageData size mismatch");

// -- Data_Message -------------------------------------------------------------
struct DyDataMessage {
    int32_t      size;   // should equal sizeof(DyDataMessage)
    int32_t      count;
    // NO char pad[4] - removed by FORCE_STRUCT_ALIGN
    DyMessageData data;
};
// 4 + 4 + 888832 = 888840
static_assert(sizeof(DyDataMessage) == 888840, "DyDataMessage size mismatch");

static constexpr int DY_MSG_SIZE      = sizeof(DyMsg);          // 8
static constexpr int DY_DATAMSG_SIZE  = sizeof(DyDataMessage);  // 888840

#pragma pack(pop)
