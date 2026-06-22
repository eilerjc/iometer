/* ######################################################################### */
/* ##                                                                     ## */
/* ##  (Iometer) / IOMemDebug.h                                           ## */
/* ##                                                                     ## */
/* ##  Job .......: Optional MFC heap-leak tracking with per-allocation   ## */
/* ##               source file + line.                                   ## */
/* ##                                                                     ## */
/* ##               In a _DEBUG build with IOMTR_SETTING_MFC_MEMALLOC_     ## */
/* ##               DEBUG defined, every "new" in a source file that      ## */
/* ##               includes this header (last) records the file+line of  ## */
/* ##               the allocation, so the leak report MFC dumps at exit   ## */
/* ##               names where the leaked block was allocated. The flag  ## */
/* ##               is undefined by default; a developer hunting a leak   ## */
/* ##               defines it in one place to instrument the whole build. ## */
/* ##                                                                     ## */
/* ##  Replaces ..: The per-.cpp boilerplate that ~28 source files each   ## */
/* ##               used to carry verbatim:                               ## */
/* ##                                                                     ## */
/* ##                 #if defined(IOMTR_OS_WIN32)||defined(IOMTR_OS_WIN64) ## */
/* ##                 #ifdef IOMTR_SETTING_MFC_MEMALLOC_DEBUG             ## */
/* ##                 #define new DEBUG_NEW                               ## */
/* ##                 #undef THIS_FILE                                    ## */
/* ##                 static char THIS_FILE[] = __FILE__;                 ## */
/* ##                 #endif                                              ## */
/* ##                 #endif                                              ## */
/* ##                                                                     ## */
/* ##               MFC's debug operator new takes the file name as a     ## */
/* ##               plain string -                                        ## */
/* ##                 operator new(size_t, LPCSTR fileName, int line)     ## */
/* ##               (afx.h) - so __FILE__, which expands to the INCLUDING ## */
/* ##               .cpp at each "new" call site, identifies the file     ## */
/* ##               just as well as the old per-file                      ## */
/* ##               "static char THIS_FILE[] = __FILE__" did. THIS_FILE   ## */
/* ##               was only a code-size optimization (one filename       ## */
/* ##               string per file instead of one per call site) and is  ## */
/* ##               not worth duplicating the whole block into every      ## */
/* ##               source file, so the mechanism now lives here once.    ## */
/* ##                                                                     ## */
/* ##  IMPORTANT ..: This header does "#define new ...", so it MUST be the ## */
/* ##               LAST #include in any .cpp that uses it - after        ## */
/* ##               stdafx.h and every other header - or "new" would be   ## */
/* ##               redefined while system/MFC headers are still being    ## */
/* ##               compiled. It is gated on _DEBUG because MFC only       ## */
/* ##               declares the file/line operator new in debug builds.  ## */
/* ##                                                                     ## */
/* ######################################################################### */
#ifndef ___IOMEMDEBUG_H_DEFINED___
#define ___IOMEMDEBUG_H_DEFINED___

#if (defined(IOMTR_OS_WIN32) || defined(IOMTR_OS_WIN64)) && \
    defined(_DEBUG) && defined(IOMTR_SETTING_MFC_MEMALLOC_DEBUG)

// Route "new" through MFC's debug operator new(size_t, LPCSTR, int), passing the
// including .cpp's __FILE__ (and __LINE__). Two steps so the "new" produced by
// the expansion is not itself re-expanded.
#define IOMTR_DEBUG_NEW new(__FILE__, __LINE__)
#define new IOMTR_DEBUG_NEW

#endif

#endif				// ___IOMEMDEBUG_H_DEFINED___
