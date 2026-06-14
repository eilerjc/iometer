#pragma once

#include <ostream>
#include <string>
#include <vector>
#include "IcfDocument.h"   // the pure-data section models (shared with the loaders)

// Section WRITERS for ICF configuration files - the byte-for-byte counterpart of
// the IcfDocument loaders. These are faithful ports of the MFC SaveConfig methods
// (CPageSetup / CPageDisplay / AccessSpecList / Worker+Manager+ManagerList), so a
// front-end can emit the exact canonical 1.1.0 format the MFC GUI has always
// written, from the same pure-data structs the loaders produce.
//
// Each writer takes a std::ostream so the caller controls the sink (an ofstream
// for a real save - text mode yields the historical CRLF on Windows - or an
// ostringstream for byte tests). Output uses '\n'; the stream's mode decides the
// on-disk line ending. No Qt/MFC types: usable from both front-ends.

namespace iocore {

class IcfWriter {
public:
    // 'TEST SETUP (port of CPageSetup::SaveConfig). The 'Run Time and cycling
    // rows use the historical left-justified setw(11) column padding.
    static void writeTestSetup(std::ostream &os, const IcfTestSetup &ts);

    // 'RESULTS DISPLAY (port of CPageDisplay::SaveConfig). Writes the bars in the
    // order given; the MFC adapter supplies all NUM_STATUS_BARS, the Qt adapter
    // supplies whatever it tracks.
    static void writeResultsDisplay(std::ostream &os, const IcfDisplaySettings &ds);

    // 'ACCESS SPECIFICATIONS (port of AccessSpecList::SaveConfig). Writes every
    // spec in order (the caller omits the idle spec, as MFC does).
    static void writeAccessSpecs(std::ostream &os, const std::vector<IcfAccessSpec> &specs);

    // 'MANAGER LIST (port of ManagerList/Manager/Worker::SaveConfig). saveAspecs/
    // saveTargets gate the per-worker 'Assigned access specs / 'Target assignments
    // blocks, exactly like the MFC flags. Network-client workers are the caller's
    // concern (MFC skips them); pass only the workers that should be written.
    static void writeManagerList(std::ostream &os,
                                 const std::vector<IcfManagerConfig> &managers,
                                 bool saveAspecs = true, bool saveTargets = true);

    // The "Version <v> " line MFC writes at the top and bottom of every ICF.
    static void writeVersionLine(std::ostream &os, const std::string &version);

private:
    static void writeWorker(std::ostream &os, const IcfWorkerConfig &w,
                            bool saveAspecs, bool saveTargets);
};

} // namespace iocore
