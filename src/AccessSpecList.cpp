/* ######################################################################### */
/* ##                                                                     ## */
/* ##  (Iometer) / AccessSpecList.cpp                                     ## */
/* ##                                                                     ## */
/* ## ------------------------------------------------------------------- ## */
/* ##                                                                     ## */
/* ##  Job .......: Implementation of the AccessSpecList class that       ## */
/* ##               is in charge of keeping track of all the access       ## */
/* ##               specifications available to the user.                 ## */
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
/* ##  Remarks ...: - Each access spec is defined by a Test_Spec          ## */
/* ##                 structure. There is always exactly one              ## */
/* ##                 AccessSpecList object in an instance of Iometer     ## */
/* ##                 (CGalileoApp::access_spec_list).                    ## */
/* ##                                                                     ## */
/* ## ------------------------------------------------------------------- ## */
/* ##                                                                     ## */
/* ##  Changes ...: 2004-04-24 (daniel.scheibli@edelbyte.org)             ## */
/* ##               - Added a large set of Global Access Specifications   ## */
/* ##                 that can be used as a starting point. The values    ## */
/* ##                 was taken from the same ICF's provided by           ## */
/* ##                 Richard Riggs.                                      ## */
/* ##               2003-10-17 (daniel.scheibli@edelbyte.org)             ## */
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
#include "AccessSpecList.h"
#include "GalileoApp.h"
#include "GalileoView.h"
#include "core/AccessSpecCatalog.h"	// shared smart-name formatting (iocore)
#include "core/IcfDocument.h"	// shared ICF access-spec loader (iocore)
#include "core/IcfWriter.h"	// shared ICF section writers (iocore)

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

//
// Initializing the pointer list.
//
AccessSpecList::AccessSpecList()
{
	// Initialize the pointer array.
	spec_list.SetSize(INITIAL_ARRAY_SIZE, ARRAY_GROW_STEP);

	// Insert the idle spec as the first in the row
	InsertIdleSpec();

	// Insert a default spec that will be assigned
	// (in former days, this one was assigned to all workers that log in)
	Test_Spec *spec = New();

	strcpy(spec->name, "Default");
	spec->default_assignment = FALSE;	// [AssignAll|AssignDisk|...]

	// Insert the default specifications
	// (based on the file provided by Richard Riggs)
	InsertDefaultSpecs();
}

//
// Inserts the idle spec.
//
void AccessSpecList::InsertIdleSpec()
{
	// Set idle spec.
	Test_Spec *spec = New();

	_snprintf(spec->name, MAX_NAME, IDLE_NAME);
	spec->access[0].of_size = IOERROR;	// indicates the end of the spec.
	// Note that the end is the first
	// line of the spec, making it
	// empty.
}

//
// Inserts the different default specifications.
//
void AccessSpecList::InsertDefaultSpecs()
{
	enum {ITER_ONE=0, ITER_ALL=1, ITER_MAX=2};
	Test_Spec *spec;
	int index = 0;
	int iterations = 0;

	// Runs through 2 iterations, one for individual access specs, and one for the all-in-one
	while (iterations < ITER_MAX)
	{
		spec = New(); 
		index = 0;

		if (iterations == ITER_ONE) _snprintf(spec->name, MAX_NAME, "512 B; 100%% Read; 0%% random");
		else 	_snprintf(spec->name, MAX_NAME, "All in one");

		spec->access[index].of_size = 100;
		spec->access[index].size = 512;
		spec->access[index].reads = 100;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

#ifndef IOMTR_SETTING_NO_OLD_ACCESS_SPEC

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "512 B; 75%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 512;
		spec->access[index].reads = 75;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;
#endif
		if (iterations == ITER_ONE) 
		{
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "512 B; 50%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 512;
		spec->access[index].reads = 50;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

#ifndef IOMTR_SETTING_NO_OLD_ACCESS_SPEC
		if (iterations == ITER_ONE) 
		{
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "512 B; 25%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 512;
		spec->access[index].reads = 25;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;
#endif

		if (iterations == ITER_ONE) 
		{
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "512 B; 0%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 512;
		spec->access[index].reads = 0;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

	// 4096 Bytes / 4 Kilo Bytes

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "4 KiB; 100%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 4096;
		spec->access[index].reads = 100;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

#ifndef IOMTR_SETTING_NO_OLD_ACCESS_SPEC

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "4 KiB; 75%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 4096;
		spec->access[index].reads = 75;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;
#endif

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "4 KiB; 50%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 4096;
		spec->access[index].reads = 50;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

#ifndef IOMTR_SETTING_NO_OLD_ACCESS_SPEC

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "4 KiB; 25%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 4096;
		spec->access[index].reads = 25;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;
#endif

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "4 KiB; 0%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 4096;
		spec->access[index].reads = 0;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

#ifdef IOMTR_SETTING_USE_NEW_ACCESS_SPEC
	// 4096 Bytes / 4 Kilo Bytes

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "4 KiB aligned; 100%% Read; 100%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 4096;
		spec->access[index].reads = 100;
		spec->access[index].random = 100;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 4096;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "4 KiB aligned; 50%% Read; 100%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 4096;
		spec->access[index].reads = 50;
		spec->access[index].random = 100;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 4096;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "4 KiB aligned; 0%% Read; 100%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 4096;
		spec->access[index].reads = 0;
		spec->access[index].random = 100;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 4096;
		spec->access[index].reply = 0;
#endif
	
		// 16384 Bytes / 16 Kilo Bytes

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "16 KiB; 100%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 16384;
		spec->access[index].reads = 100;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

#ifndef IOMTR_SETTING_NO_OLD_ACCESS_SPEC
		
		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "16 KiB; 75%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 16384;
		spec->access[index].reads = 75;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;
#endif

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "16 KiB; 50%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 16384;
		spec->access[index].reads = 50;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

#ifndef IOMTR_SETTING_NO_OLD_ACCESS_SPEC

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "16 KiB; 25%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 16384;
		spec->access[index].reads = 25;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;
#endif

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "16 KiB; 0%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 16384;
		spec->access[index].reads = 0;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

	// 32768 Bytes / 32 Kilo Bytes
#ifndef IOMTR_SETTING_NO_OLD_ACCESS_SPEC

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "32 KiB; 100%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 32768;
		spec->access[index].reads = 100;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "32 KiB; 75%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 32768;
		spec->access[index].reads = 75;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "32 KiB; 50%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 32768;
		spec->access[index].reads = 50;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "32 KiB; 25%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 32768;
		spec->access[index].reads = 25;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "32 KiB; 0%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 32768;
		spec->access[index].reads = 0;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;
#endif

#ifdef IOMTR_SETTING_USE_NEW_ACCESS_SPEC

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "64 KiB; 100%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 65536;
		spec->access[index].reads = 100;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "64 KiB; 50%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 65536;
		spec->access[index].reads = 50;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "64 KiB; 0%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 65536;
		spec->access[index].reads = 0;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "256 KiB; 100%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 262144;
		spec->access[index].reads = 100;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "256 KiB; 50%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 262144;
		spec->access[index].reads = 50;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

		if (iterations == ITER_ONE) 
		{ 
			spec = New(); 
			_snprintf(spec->name, MAX_NAME, "256 KiB; 0%% Read; 0%% random");
		}
		else index++;

		spec->access[index].of_size = 100;
		spec->access[index].size = 262144;
		spec->access[index].reads = 0;
		spec->access[index].random = 0;
		spec->access[index].delay = 0;
		spec->access[index].burst = 1;
		spec->access[index].align = 0;
		spec->access[index].reply = 0;

#endif  // #ifdef IOMTR_SETTING_USE_NEW_ACCESS_SPEC

		if (iterations == ITER_ALL) 
		{
			// Terminate the spec for the all_in_one case
			spec->access[++index].of_size = IOERROR;

			// Fix up the percent access for each entry based on the total
			// as evenly as possible.
			for (int i=0; i<index; i++)
			{
				if (100%index && (i < 100%index))
					spec->access[i].of_size = 100/index + 1;
				else
					spec->access[i].of_size = 100/index;
			}
		}
	
		iterations++;
	} // while
}

//
// Removes all AccessSpecObjects form memory and removes all references 
// from the pointer array.
//
AccessSpecList::~AccessSpecList()
{
	DeleteAll();
}

//
// Creates a new AccessSpecObject and adds a pointer to it to the end of
// the pointer array.
//
Test_Spec *AccessSpecList::New()
{
	Test_Spec *spec = new Test_Spec;

	if (!spec)
		return NULL;
	// Make the default new access spec
	InitAccessSpecLine(&(spec->access[0]));
	spec->access[1].of_size = IOERROR;

	// Set the access spec to not load by default.
	spec->default_assignment = FALSE;

	// Name the new access spec.
	NextUntitled(spec->name);

	// Add the new access spec to the end of the array.
	spec_list.Add(spec);

	// Return the index in the pointer array of the newly added object's pointer.
	return spec;
}

//
// Creates a copy of the specified access spec, and returns a copy.
//
Test_Spec *AccessSpecList::Copy(Test_Spec * source_spec)
{
	int copy_number;
	CString name;
	Test_Spec *spec;

	// Create a new spec and check the validity of source_spec.
	if (!(spec = New()) || IndexByRef(source_spec) == IOERROR)
		return NULL;

	// Copy the source spec into the newly created one.
	memcpy(spec, source_spec, sizeof Test_Spec);

	// Assign a unique name: Copy of 'name of source spec' ('unique copy number').
	name.Format("Copy of %s", spec->name);
	copy_number = 1;
	do {
		snprintf(spec->name, MAX_NAME, "%s (%d)", (LPCTSTR)name, copy_number++);
	} while (RefByName(spec->name) != spec);

	return spec;
}

//
// Deletes the specified Test_Spec object, as well as the related entry in 
// the pointer array.  Also removes any reference that spec by any worker.
//
void AccessSpecList::Delete(Test_Spec * spec)
{
	// Check for an invalid pointer.
	if (IndexByRef(spec) == IOERROR)
		return;

	// Remove any references to this access spec from the workers.
	theApp.manager_list.RemoveAccessSpec(spec);

	// Remove the reference to the access spec from the array.
	spec_list.RemoveAt(IndexByRef(spec));

	// Remove the access spec from memory.
	delete spec;
}

//
// Deletes all the access spec objects and their related entries.
//
void AccessSpecList::DeleteAll()
{
	while (spec_list.GetSize()) {
		delete spec_list[spec_list.GetUpperBound()];

		spec_list.RemoveAt(spec_list.GetUpperBound());
	}
	theApp.manager_list.RemoveAllAccessSpecs();
}

//
// Returns the pointer to the access spec object specified by the index.
//
Test_Spec *AccessSpecList::Get(int index)
{
	if (index >= 0 && index < spec_list.GetSize())
		return spec_list[index];
	else
		return NULL;
}

//
// Returns the number of entries in the array.
//
int AccessSpecList::Count()
{
	return (int)spec_list.GetSize();
}

//
// Returns a pointer to a spec of a given name, if it exists.
//
Test_Spec *AccessSpecList::RefByName(const char *check_name)
{
	Test_Spec *spec;
	int spec_count = (int)spec_list.GetSize();

	for (int s = 0; s < spec_count; s++) {
		spec = Get(s);
		if (strcasecmp(spec->name, check_name) == 0)
			return spec;
	}
	return NULL;
}

//
// Returns the index of a spec given a reference to that spec.
// This function is used to validate spec pointers.
// Also used in Delete() to index into the array, and in 
// Save() to determine which specs where running. 
//
int AccessSpecList::IndexByRef(const Test_Spec * spec)
{
	int spec_count = (int)spec_list.GetSize();

	for (int index = 0; index < spec_count; index++) {
		if (Get(index) == spec)
			return index;
	}
	return IOERROR;
}

//
// Saving the access specification list to a result file.
// Saves only the currently active access specs.
//
BOOL AccessSpecList::SaveResults(ostream & outfile)
{
	int i, j;
	int access_count, manager_count, worker_count, current_access_index;
	Manager *mgr;
	Worker *wkr;
	BOOL *spec_active;
	Test_Spec *spec;

	outfile << "'Access specifications" << endl;

	access_count = Count();
	spec_active = new BOOL[access_count];

	// Determine which access specs are active for the current test
	// Start by marking all access specs inactive.
	for (i = 1; i < access_count; i++) {
		spec_active[i] = FALSE;
	}

	current_access_index = theApp.pView->GetCurrentAccessIndex();
	// Now go through all the workers and mark each worker's current access spec 
	// as active, IF the worker is actually doing anything in this test.
	manager_count = theApp.manager_list.ManagerCount();
	for (i = 0; i < manager_count; i++) {
		mgr = theApp.manager_list.GetManager(i);
		worker_count = mgr->WorkerCount();
		for (j = 0; j < worker_count; j++) {
			wkr = mgr->GetWorker(j);
			if (wkr->ActiveInCurrentTest()) {
				spec_active[IndexByRef(wkr->GetAccessSpec(current_access_index))] = TRUE;
			}
		}
	}

	// Save all the active access specs except the idle spec.
	for (i = 1; i < access_count; i++) {
		if (!spec_active[i])
			continue;

		spec = Get(i);

		outfile << "'Access specification name,default assignment" << endl;
		outfile << spec->name << "," << spec->default_assignment << endl;

		// Write access specifications to a file, data comma separated.
		outfile << "'size,% of size,% reads,% random,delay,burst,align,reply" << endl;

		for (int line_index = 0; line_index < MAX_ACCESS_SPECS; line_index++)
		{
			if (spec->access[line_index].of_size == IOERROR)
				break;

			outfile
				<< spec->access[line_index].size << ","
				<< spec->access[line_index].of_size << ","
				<< spec->access[line_index].reads << ","
				<< spec->access[line_index].random << ","
				<< spec->access[line_index].delay << ","
				<< spec->access[line_index].burst << ","
				<< spec->access[line_index].align << "," 
				<< spec->access[line_index].reply << ",";
			outfile << endl;
		}
	}

	outfile << "'End access specifications" << endl;

	delete [] spec_active;

	return TRUE;
}

//
// Saves the access specification list to a config file.
// Saves ALL specs, whether they are active or not.
// It is tempting to combine this with SaveResults, but changes to one should
// not generally affect the other.  At one point, the output of these two
// functions was identical, but there are differences now, and there will
// continue to be more in the future.
//
BOOL AccessSpecList::SaveConfig(ostream & outfile)
{
	int i, line_index;
	int access_count = Count();
	Test_Spec *spec;

	// The 'ACCESS SPECIFICATIONS byte format is the shared writer
	// (iocore::IcfWriter); this adapter gathers the live specs (skipping the idle
	// spec 0) into the structs, keeping the default-assignment validation.
	std::vector<iocore::IcfAccessSpec> specs;

	for (i = 1; i < access_count; i++) {
		spec = Get(i);

		iocore::IcfAccessSpec s;
		s.name = spec->name;

		switch (spec->default_assignment) {
		case AssignNone: s.defaultAssignment = 0; break;
		case AssignAll:  s.defaultAssignment = 1; break;
		case AssignDisk: s.defaultAssignment = 2; break;
		case AssignNet:  s.defaultAssignment = 3; break;
		default:
			ErrorMessage("Error saving access specification list.  Access specification "
				     + (CString) spec->name + " has an illegal default assignment.");
			return FALSE;
		}

		for (line_index = 0; line_index < MAX_ACCESS_SPECS; line_index++) {
			if (spec->access[line_index].of_size == IOERROR)
				break;
			iocore::IcfAccessSpecLine l;
			l.size   = spec->access[line_index].size;
			l.ofSize = spec->access[line_index].of_size;
			l.reads  = spec->access[line_index].reads;
			l.random = spec->access[line_index].random;
			l.delay  = spec->access[line_index].delay;
			l.burst  = spec->access[line_index].burst;
			l.align  = spec->access[line_index].align;
			l.reply  = spec->access[line_index].reply;
			s.lines.push_back(l);
		}

		specs.push_back(s);
	}

	iocore::IcfWriter::writeAccessSpecs(outfile, specs);

	return TRUE;
}

//
// Checks the config file version number and calls the appropriate load function.
// Also handles the replace flag.
//
// If replace is TRUE, all access specs are removed before the restore,
// otherwise, the loaded specs are merged with the preexisting specs.
//
BOOL AccessSpecList::LoadConfig(const CString & infilename, BOOL replace)
{
	if (replace) {
		// Clear memory and display before loading accesses.
		DeleteAll();
		InsertIdleSpec();
	}

	// Parsing (both the modern format and the pre-1998.05.21 numeric format,
	// version-dispatched) is shared core logic (iocore::IcfDocument). This
	// adapter applies the parsed specs to the live list with the same
	// replace-by-name semantics as the old in-place parser, and replays the
	// recorded MFC error dialogs.
	iocore::IcfDocument doc((LPCTSTR) infilename);

	std::vector<std::string> existing;
	for (int e = 0; e < Count(); e++)
		existing.push_back(Get(e)->name);

	std::vector<iocore::IcfAccessSpec> loaded;
	const bool ok = doc.loadAccessSpecs(loaded, &existing);

	for (size_t e = 0; e < doc.errors().size(); e++)
		ErrorMessage(CString(doc.errors()[e].c_str()));

	if (!ok)
		return FALSE;

	// Apply to the live list. A spec whose name matches an existing one
	// replaces it in place (worker references stay valid), like before.
	for (size_t s = 0; s < loaded.size(); s++) {
		const iocore::IcfAccessSpec & ls = loaded[s];

		Test_Spec *spec = RefByName(ls.name.c_str());
		if (!spec) {
			spec = New();
			if (!spec) {
				ErrorMessage("Error while loading file.  Out of memory.  "
					     "Error occured in AccessSpecList::LoadConfig()");
				return FALSE;
			}
		}

		_snprintf(spec->name, MAX_NAME, "%s", ls.name.c_str());
		spec->default_assignment = ls.defaultAssignment;

		int line_index = 0;
		for (size_t li = 0; li < ls.lines.size() && line_index < MAX_ACCESS_SPECS; li++, line_index++) {
			const iocore::IcfAccessSpecLine & l = ls.lines[li];
			spec->access[line_index].size    = l.size;
			spec->access[line_index].of_size = l.ofSize;
			spec->access[line_index].reads   = l.reads;
			spec->access[line_index].random  = l.random;
			spec->access[line_index].delay   = l.delay;
			spec->access[line_index].burst   = l.burst;
			spec->access[line_index].align   = l.align;
			spec->access[line_index].reply   = l.reply;
		}
		// Mark the end of the access spec.
		if (line_index <= MAX_ACCESS_SPECS)
			spec->access[line_index].of_size = IOERROR;
	}

	// Old-format files: if replacing, assign the default specs to workers.
	// (New-format files don't need this - the Worker constructor assigns
	// defaults and Manager::LoadConfig removes them when appropriate.)
	if (doc.readVersion() < 19980521 && replace)
		theApp.manager_list.AssignDefaultAccessSpecs();

	return TRUE;
}

//
// This function initializes a line of an access spec to the default values.
//
void AccessSpecList::InitAccessSpecLine(Access_Spec * spec_line)
{
	spec_line->size = 2048;
	spec_line->of_size = 100;
	spec_line->random = 100;
	spec_line->reads = 67;
	spec_line->delay = 0;
	spec_line->burst = 1;
	spec_line->align = spec_line->size;
	spec_line->reply = 0;
}

//
// This function will return an integer that will be the next sequential 
// n for naming "Untitled n" access specs.
//
void AccessSpecList::NextUntitled(char *name)
{
	int next_number = 0;
	CString current_name;

	// Counting the number of "Untitled *" specs.
	for (int index = 0; index < spec_list.GetSize(); index++) {
		current_name = Get(index)->name;
		if (current_name.Find("Untitled") == 0) {
			++next_number;
		}
	}

	// Ensure that names are not duplicated.
	do {
		// We have the current number, so we add 1 for the next number.
		snprintf(name, MAX_NAME, "Untitled %d", ++next_number);
	}
	while (RefByName(name));
}

//
// Assigning a reasonable name to an older access spec that doesn't have one.
//
void AccessSpecList::SmartName(Test_Spec * spec)
{
	CString name;
	int spec_number = 1;
	int name_size;

	// Verify that smart naming is possible.
	if (spec->access[1].of_size != IOERROR)
		return;		// use existing name

	// Formatting is shared core logic (also used by the Qt GUI's library).
	name = iocore::smartNameText((int)spec->access[0].size,
				     spec->access[0].random, spec->access[0].reads).c_str();

	// Test to ensure no duplicate names
	name_size = name.GetLength();
	while (RefByName((LPCTSTR) name)) {
		name.Format("%s %d", name.Left(name_size), ++spec_number);
	}
	snprintf(spec->name, MAX_NAME, "%s", (LPCTSTR)name);
}
