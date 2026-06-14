/* ######################################################################### */
/* ##                                                                     ## */
/* ##  (Iometer) / ManagerMap.cpp                                         ## */
/* ##                                                                     ## */
/* ## ------------------------------------------------------------------- ## */
/* ##                                                                     ## */
/* ##  Job .......: Implementation of the ManagerMap class for Iometer,   ## */
/* ##               which maintains the mapping between managers in an    ## */
/* ##               .ICF file and the current Manager objects.            ## */
/* ##                                                                     ## */
/* ## ------------------------------------------------------------------- ## */
/* ##                                                                     ## */
/* ##  Intel Open Source License                                          ## */
/* ##                                                                     ## */
/* ##  Copyright (c) 2001 Intel Corporation                               ## */
/* ##  All rights reserved.                                               ## */
/* ##  Redistribution and use in source and binary forms, with or         ## */
/* ##  without modification, are permitted provided that the following    ## */
/* ##  conditions are met:                                                ## */
/* ##                                                                     ## */
/* ##  Redistributions of source code must retain the above copyright     ## */
/* ##  notice, this list of conditions and the following disclaimer.      ## */
/* ##                                                                     ## */
/* ##  Redistributions in binary form must reproduce the above copyright  ## */
/* ##  notice, this list of conditions and the following disclaimer in    ## */
/* ##  the documentation and/or other materials provided with the         ## */
/* ##  distribution.                                                      ## */
/* ##                                                                     ## */
/* ##  Neither the name of the Intel Corporation nor the names of its     ## */
/* ##  contributors may be used to endorse or promote products derived    ## */
/* ##  from this software without specific prior written permission.      ## */
/* ##                                                                     ## */
/* ##  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             ## */
/* ##  CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,      ## */
/* ##  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           ## */
/* ##  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           ## */
/* ##  DISCLAIMED. IN NO EVENT SHALL THE INTEL OR ITS  CONTRIBUTORS BE    ## */
/* ##  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,   ## */
/* ##  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,           ## */
/* ##  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,    ## */
/* ##  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY    ## */
/* ##  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR     ## */
/* ##  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT    ## */
/* ##  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY    ## */
/* ##  OF SUCH DAMAGE.                                                    ## */
/* ##                                                                     ## */
/* ## ------------------------------------------------------------------- ## */
/* ##                                                                     ## */
/* ##  Remarks ...: <none>                                                ## */
/* ##                                                                     ## */
/* ## ------------------------------------------------------------------- ## */
/* ##                                                                     ## */
/* ##  Changes ...: 2003-10-17 (daniel.scheibli@edelbyte.org)             ## */
/* ##               - Moved to the use of the IOMTR_[OSFAMILY|OS|CPU]_*   ## */
/* ##                 global defines.                                     ## */
/* ##               - Integrated the License Statement into this header.  ## */
/* ##               2003-04-25 (daniel.scheibli@edelbyte.org)             ## */
/* ##               - Updated the global debug flag (_DEBUG) handling     ## */
/* ##                 of the source file (check for platform etc.).       ## */
/* ##               - Added new header holding the changelog.             ## */
/* ##                                                                     ## */
/* ######################################################################### */

#include "stdafx.h"
#include "ManagerMap.h"
#include "GalileoApp.h"

// Needed for MFC Library support for assisting in finding memory leaks
//
// NOTE: Based on the documentation[1] I found, it should be enough to have
//       a "#define new DEBUG_NEW" statement for the case, that we are
//       running Windows. There should be no need for checking the _DEBUG
//       flag and no need for redefiniting the THIS_FILE string. Maybe there
//       will be a MFC hacker who could advice here.
//       [1] = http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclib/html/_mfc_debug_new.asp
//
#if defined(IOMTR_OS_WIN32) || defined(IOMTR_OS_WIN64)
#ifdef IOMTR_SETTING_MFC_MEMALLOC_DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

ManagerMap::ManagerMap()
{
	Reset();
}

//
// Prepare the ManagerMap for reuse.
//
void ManagerMap::Reset()
{
	m_core.reset();
	waiting_dialog.Reset();
}

// Convert an MFC CString to std::string for the core layer.
static std::string ToStd(const CString & s)
{
	return std::string((LPCTSTR) s);
}

//
// Add a manager to the list
//
// If any added manager's Manager* is NULL, the ManagerMap will be put
// in a waiting list state, meaning that IsWaitingList will return TRUE
// until ManagerLoggedIn is called with a valid pointer for that manager.
// The manager will also be added to the waiting list if necessary.
//
void ManagerMap::Store(const CString & name, const int id, const CString & address, Manager * mgr)
{
	m_core.store(ToStd(name), id, ToStd(address), mgr);

	// Add this manager to the waiting list if necessary.
	if (mgr == NULL)
		waiting_dialog.AddWaitingManager(name, address);
}

//
// Retrieve a manager pointer matching the specified criteria
//
// NULL if no matching manager was found.
//
Manager *ManagerMap::Retrieve(const CString & name, const int id)
{
	// Make sure the ManagerMap isn't in a waiting list state (DEBUG only)
	ASSERT(!IsWaitingList());

	return (Manager *) m_core.retrieve(ToStd(name), id);
}

//
// When a manager logs in, call this function to find the first matching
// entry in the map and store the new manager's pointer there.
// (This is only necessary if the manager list is in a waiting state.)
//
// Returns FALSE if no matching manager could be found.
//
BOOL ManagerMap::ManagerLoggedIn(const CString & name, const CString & address, Manager * mgr)
{
	if (m_core.managerLoggedIn(ToStd(name), ToStd(address), mgr)) {
		// Remove this manager from the waiting list.
		(void)waiting_dialog.RemoveWaitingManager(name, address);
		return TRUE;
	}
	return FALSE;
}

//
// If there is only one manager in the ManagerMap and it is unassigned
// (mgr pointer is NULL), assign it the given manager pointer, return TRUE.
// Otherwise, return FALSE.
//
BOOL ManagerMap::SetIfOneManager(Manager * mgr)
{
	return m_core.setIfOneManager(mgr) ? TRUE : FALSE;
}

//
// Are we waiting for any managers to log in before restoring
// the manager configuration from a saved file?
//
BOOL ManagerMap::IsWaitingList()
{
	return m_core.isWaitingList() ? TRUE : FALSE;
}

//
// Use this function to determine whether to keep or remove a
// manager when restoring manager settings from a config file.
// If the passed-in manager pointer matches one used in the
// ManagerMap, the manager is needed for restoring the config
// file.  Otherwise, it should be removed.
//
BOOL ManagerMap::IsThisManagerNeeded(const Manager * const mgr)
{
	return m_core.isManagerNeeded((iocore::ManagerMap::Handle) const_cast<Manager *>(mgr))
	    ? TRUE : FALSE;
}

//
// Find all unassigned managers (mgr ptrs are NULL) with the local
// network address and spawn Dynamos with the appropriate names.
//
void ManagerMap::SpawnLocalManagers()
{
	// Get the local machine's NetBIOS/NT name first.
	// (IsAddressLocal would also match IP addresses, which is undesirable for the
	// name-based local check; the core does both, given the name and an isLocal fn.)
	CString nt_name;
	DWORD namelength = MAX_NETWORK_NAME;
	::GetComputerName(nt_name.GetBuffer(MAX_NETWORK_NAME), &namelength);
	nt_name.ReleaseBuffer();

	// Core decides WHICH unassigned local managers to spawn (and with what -n);
	// this wrapper performs the actual spawn via LaunchDynamo().
	std::vector<iocore::ManagerMap::SpawnRequest> requests = m_core.localManagersToSpawn(
	    [](const std::string & addr) -> bool {
		    return theApp.IsAddressLocal(CString(addr.c_str())) != FALSE;
	    },
	    ToStd(nt_name));

	for (size_t i = 0; i < requests.size(); i++) {
		if (requests[i].nameArg.empty())
			theApp.LaunchDynamo();	// spawn a Dynamo with local defaults
		else
			theApp.LaunchDynamo(" -n " + CString(requests[i].nameArg.c_str()));
	}
}

//
// Show the waiting list dialog box, return immediately.
//
void ManagerMap::ShowWaitingList(const CString & infilename, BOOL * flags, BOOL replace)
{
	waiting_dialog.Create(infilename, flags, replace);
}
