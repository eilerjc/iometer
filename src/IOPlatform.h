/* ######################################################################### */
/* ##                                                                     ## */
/* ##  (Iometer) / IOPlatform.h                                           ## */
/* ##                                                                     ## */
/* ##  Job .......: Derive the Operating System FAMILY from the selected  ## */
/* ##               Operating System.                                     ## */
/* ##                                                                     ## */
/* ##               A build selects exactly one IOMTR_OS_* define (the    ## */
/* ##               OS). The OS family (IOMTR_OSFAMILY_WINDOWS / _UNIX /   ## */
/* ##               _NETWARE) is IMPLIED by the OS and derived here, so a  ## */
/* ##               build no longer has to hand-pass a redundant - and    ## */
/* ##               possibly mismatched - IOMTR_OSFAMILY_* define.         ## */
/* ##                                                                     ## */
/* ##               A build that still passes an IOMTR_OSFAMILY_* that     ## */
/* ##               DISAGREES with the OS will define two families and so  ## */
/* ##               trip IOCommon.h's "exactly one OS family" check.       ## */
/* ##                                                                     ## */
/* ##               This header has no dependencies and only #defines, so  ## */
/* ##               it is safe to include FIRST from anywhere that needs   ## */
/* ##               the family - including pack.h/unpack.h, which are      ## */
/* ##               pulled in before IOCommon.h. (The OS define itself is  ## */
/* ##               still supplied by the build; only the family, which is ## */
/* ##               purely a function of the OS, is derived.)              ## */
/* ##                                                                     ## */
/* ######################################################################### */
#ifndef ___IOPLATFORM_H_DEFINED___
#define ___IOPLATFORM_H_DEFINED___

// Defined to 1 (not empty) to exactly match a command-line -D / case
// IOMTR_OSFAMILY_* define, so even the few spots that test the family WITHOUT
// defined() - e.g. "#elif(IOMTR_OSFAMILY_WINDOWS)" - keep working.
#if defined(IOMTR_OS_WIN32) || defined(IOMTR_OS_WIN64)
#ifndef IOMTR_OSFAMILY_WINDOWS
#define IOMTR_OSFAMILY_WINDOWS 1
#endif
#elif defined(IOMTR_OS_LINUX) || defined(IOMTR_OS_OSX) || defined(IOMTR_OS_SOLARIS)
#ifndef IOMTR_OSFAMILY_UNIX
#define IOMTR_OSFAMILY_UNIX 1
#endif
#elif defined(IOMTR_OS_NETWARE)
#ifndef IOMTR_OSFAMILY_NETWARE
#define IOMTR_OSFAMILY_NETWARE 1
#endif
#endif

#endif				// ___IOPLATFORM_H_DEFINED___
