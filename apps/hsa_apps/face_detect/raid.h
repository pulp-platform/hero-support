/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                   		Imaging Division
 *------------------------------------------------------------------------------
 * $Id: raid.h.rca 1.57 Wed Sep  5 12:14:29 2012 vincent.colin-de-verdiere@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: raid.h.rca $
 * 
 *  Revision: 1.57 Wed Sep  5 12:14:29 2012 vincent.colin-de-verdiere@st.com
 *  remove reference based constructor for ImageT<T>
 * 
 *  Revision: 1.56 Wed Sep  5 11:39:17 2012 vincent.colin-de-verdiere@st.com
 *  upgrade ImageT to simplify coding of template code in particular to create temporary images in the c++ code.
 * 
 *  Revision: 1.54 Fri Jun 22 11:35:44 2012 martin.renaudin@st.com
 *  new dispatch
 *
 *  Revision: 1.53 Tue Jun 19 14:47:35 2012 vincent.colin-de-verdiere@st.com
 *  allow NULL image pointer in ImageT<T> and rename multiple method with an initial upper case.
 *  a backward #define compatiblity layer is maintained until the conversion of corresponding code is done.
 *
 *  Revision: 1.52 Fri Jun  8 15:51:09 2012 vincent.colin-de-verdiere@st.com
 *  Replace use of GetDataPtr(GetLockId()) by GetData() in GetPixelPtr() and thus suppress lock warnings.
 *
 *  Revision: 1.51 Fri Jun  8 13:47:14 2012 vincent.colin-de-verdiere@st.com
 *  add release lock code when RE-getting the data pointer in an imageT using getData()
 *
 *  Revision: 1.50 Tue Jun  5 14:23:29 2012 vincent.colin-de-verdiere@st.com
 *  Add missing comments.
 *  Fix VisualC++ code (codex #169904)
 *
 *  Revision: 1.49 Tue Jun  5 13:42:05 2012 martin.renaudin@st.com
 *  added dispatch for function with an array of input images
 *
 *  Revision: 1.48 Wed May 16 11:36:46 2012 martin.renaudin@st.com
 *  cleaned replacePlane
 *
 *  Revision: 1.47 Thu May 10 17:11:36 2012 martin.renaudin@st.com
 *  add ReplacePlane to the template image methods
 *
 *  Revision: 1.46 Wed May  9 09:38:23 2012 vincent.colin-de-verdiere@st.com
 *  * add method GetBayerColorPosition() to ImageT
 *  * add DISPATCH_IMAGE_INT_SAME_THREE() macro
 *
 *  Revision: 1.45 Wed May  2 12:10:13 2012 martin.renaudin@st.com
 *  added new method of the image class template:
 *  -getPixelPtr
 *  -getPixelOffset
 *  -copyData
 *  -copyBorder
 *  -extractBayerRing
 *  -extractKernel
 *  and a function template SortPixel
 *
 *  Revision: 1.44 Fri Apr 13 12:29:48 2012 vincent.colin-de-verdiere@st.com
 *  * add method GetImage() to obtain the _image pointer of an ImageT.
 *  * make Div2 float and double versions begin inline rather than static.
 *
 *  Revision: 1.43 Mon Mar 12 15:56:18 2012 vincent.colin-de-verdiere@st.com
 *  Add template definitions for int32/float model upgrades
 *
 *  Revision: 1.42 Thu Feb 16 16:26:01 2012 stephane.drouard@st.com
 *  Added CFGFILE_GetNextSection().
 *
 *  Revision: 1.41 Thu Mar 17 15:26:45 2011 stephane.drouard@st.com
 *  Added int64_t and uint64_t typedefs.
 *  Declared all [u]intN_t under Cython.
 *
 *  Revision: 1.40 Tue Mar 15 15:54:05 2011 stephane.drouard@st.com
 *  Added GetArrayBuffer().
 *
 *  Revision: 1.39 Wed Nov 24 13:57:18 2010 stephane.drouard@st.com
 *  Added IMG_LINK_CONST for IMG_CreateLinkToNathair().
 *
 *  Revision: 1.38 Wed Oct  6 11:14:45 2010 stephane.drouard@st.com
 *  Nathair: fixed C++ object address retrieved from a SWIGed object.
 *
 *  Revision: 1.37 Thu Sep 23 14:48:10 2010 stephane.drouard@st.com
 *  Added a function to translate a CPP exception to a Python exception to replace of the one delivered by Cython which does not acquire the GIL.
 *
 *  Revision: 1.36 Mon Sep 20 09:49:53 2010 stephane.drouard@st.com
 *  Allowed IMG_EXTRACT_KERNEL & IMG_EXTRACT_BAYER_RING macros to be usable in if statements.
 *
 *  Revision: 1.35 Mon May 17 17:44:20 2010 stephane.drouard@st.com
 *  When compiled in C++, issue a warning when IMG_image_t or CFGFILE_file_t instance has not been freed (then free it).
 *
 *  Revision: 1.34 Tue Jun  9 09:26:52 2009 sebastien.cleyet-merle@st.com
 *  Add AsSelection method to pass Python Selection object to C++
 *
 *  Revision: 1.33 Mon Jun  1 10:50:43 2009 stephane.drouard@st.com
 *  Updated comment of IMG_EXTRACT_KERNEL().
 *
 *  Revision: 1.32 Thu Mar  5 15:34:06 2009 stephane.drouard@st.com
 *  Reverted back previous modifications (GetConstData() and GetData() behave the same way).
 *  Added image address to lock ID.
 *
 *  Revision: 1.31 Mon Mar  2 17:00:26 2009 stephane.drouard@st.com
 *  Added linkType to CreateLinkToNathairImage in order to select the correct lock mechanism.
 *
 *  Revision: 1.30 Mon Feb 16 15:20:37 2009 stephane.drouard@st.com
 *  IMG_SortPixels now uses STL sort() when compiled in C++.
 *
 *  Revision: 1.29 Wed Jan 28 15:47:34 2009 stephane.drouard@st.com
 *  Cleanup.
 *
 *  Revision: 1.28 Thu Nov 20 10:06:43 2008 stephane.drouard@st.com
 *  Added GetModuleName() for NATHAIR.
 *
 *  Revision: 1.27 Wed Oct 29 17:42:44 2008 stephane.drouard@st.com
 *  Added check of the C++ type to FromSwig().
 *
 *  Revision: 1.26 Wed Oct 29 14:24:31 2008 stephane.drouard@st.com
 *  Nathair: added FromSwig() and AsImage().
 *
 *  Revision: 1.25 Wed Oct 22 10:46:05 2008 stephane.drouard@st.com
 *  Added Python module initialization.
 *
 *  Revision: 1.24 Thu Oct 16 17:16:04 2008 stephane.drouard@st.com
 *  Fixed DEBUG() macro to do nothing when compiled with NDEBUG.
 *
 *  Revision: 1.23 Thu Oct 16 10:48:44 2008 stephane.drouard@st.com
 *  Replaced VERBOSE() by WARNING()/INFO()/DEBUG() (need to delete trailing "\n").
 *  GetAppName() now only available in standalone.
 *  SetAppName() renamed in SetModuleName() (Nathair only).
 *  error.c removed and merged with utils.c (need to clean before 1st rebuild).
 *  Reworked on the documentation.
 *
 *  Revision: 1.22 Wed Oct  1 12:27:15 2008 stephane.drouard@st.com
 *  Added kernel mapping documention to IMG_EXTRACT_KERNEL().
 *
 *  Revision: 1.21 Tue Sep 30 16:35:23 2008 stephane.drouard@st.com
 *  Added YCbCr support.
 *  Fixed IMG_Check(), IMG_Clip() and IMG_Mask() (when bits == 2 or bits == 16).
 *
 *  Revision: 1.20 Tue Sep 30 13:48:25 2008 stephane.drouard@st.com
 *  Added IMG_EXTRACT_KERNEL(), IMG_EXTRACT_BAYER_RING() and IMG_SortPixels().
 *
 *  Revision: 1.19 Mon Sep 29 17:36:50 2008 stephane.drouard@st.com
 *  IMG_Shift: added round option for right shift (IMG_SHIFT_ROUND).
 *
 *  Revision: 1.18 Fri Sep  5 12:00:00 2008 olivier.lemarchand@st.com
 *  Applied dos2unix
 *
 *  Revision: 1.17 Mon Sep  1 14:24:43 2008 stephane.drouard@st.com
 *  Removed IMG_MAX_WIDTH/HEIGHT.
 *
 *  Revision: 1.16 Fri Jun 13 12:58:24 2008 stephane.drouard@st.com
 *  Added support of Visual C++ 2005 for all platforms.
 *
 *  Revision: 1.15 Wed Mar  5 17:35:52 2008 stephane.drouard@st.com
 *  Fixed some Nathair functions not documented.
 *
 *  Revision: 1.14 Thu Feb 21 15:24:26 2008 stephane.drouard@st.com
 *  Added source file and line in error messages in debug and verbose modes.
 *
 *  Revision: 1.13 Thu Feb  7 17:35:52 2008 stephane.drouard@st.com
 *  Added Nathair support.
 *
 *  Revision: 1.12 Thu Jan 24 11:33:43 2008 stephane.drouard@st.com
 *  Fixed visibility of IMG_GetBayerColorPosition().
 *
 *  Revision: 1.11 Tue Jan 15 15:05:55 2008 stephane.drouard@st.com
 *  Added check of either STANDALONE or SOFTIP defined at compile time.
 *
 *  Revision: 1.10 Tue Jan  8 10:56:22 2008 stephane.drouard@st.com
 *  Added IMG_Mask().
 *  Minor documentation updates.
 *
 *  Revision: 1.9 Fri Dec 21 17:24:06 2007 stephane.drouard@st.com
 *  Added IMG_Clip().
 *
 *  Revision: 1.8 Fri Sep 21 14:34:47 2007 stephane.drouard@st.com
 *  Renamed IMGSHIFT_COPY_MS_BITS as IMG_SHIFT_COPY_MS_BITS.
 *
 *  Revision: 1.7 Thu Aug 23 10:27:06 2007 olivier.lemarchand@st.com
 *  Added IMGSHIFT_COPY_MS_BITS macro specific to IMG_Shift()
 *
 *  Revision: 1.6 Fri Jun  1 11:21:45 2007 stephane.drouard@st.com
 *  Added IMG_HJOG(), IMG_VJOG(), IMG_BAYER_COLOR(), IMG_Duplicate() and IMG_GetBayerColorPosition().
 *  IMG_Crop(), IMG_Copy(), IMG_CopyBorder() now report the jogs to the destination image.
 *
 *  Revision: 1.5 Fri Apr 27 09:46:24 2007 stephane.drouard@st.com
 *  IMG_Check(): updated error message (coordinate, value).
 *  Updated comments.
 *
 *  Revision: 1.4 Wed Apr 18 10:17:29 2007 stephane.drouard@st.com
 *  Added module parameter to TRY_RAID_LIBRARY().
 *
 *  Revision: 1.3 Fri Apr 13 17:23:14 2007 stephane.drouard@st.com
 *  Added SoftIP environment support.
 *  Updated comments.
 *  Changed IMG_Create(): now lets the buffer uninitialized (performance, not backward compatible).
 *  Changed IMG_Copy(): now supports overlapping regions.
 *
 *  Revision: 1.2 Wed Apr  4 16:31:06 2007 stephane.drouard@st.com
 *  Adapted to new RAID structure.
 *
 *  Revision: 1.1 Wed Apr  4 15:30:07 2007 olivier.lemarchand@st.com
 *  mvfile on Wed Apr  4 15:30:07 2007 by user olema.
 *  Originated from sync://syncidd1.gnb.st.com:2800/Projects/RAID_LIB/standalone.h;1.26
 *
 *  Revision: 1.26 Tue Apr  3 12:01:39 2007 stephane.drouard@st.com
 *  Added support of IMG_COLOR_2G (merge of 2 Bayer greens) for IMG_ExtractPlane().
 *
 *  Revision: 1.25 Fri Mar 23 17:17:08 2007 stephane.drouard@st.com
 *  Adjusted jog order (h then v).
 *
 *  Revision: 1.24 Fri Mar 23 16:43:12 2007 stephane.drouard@st.com
 *  Added IMG_Copy() and IMG_CopyBorder().
 *
 *  Revision: 1.23 Fri Mar 23 11:22:13 2007 stephane.drouard@st.com
 *  Removed Error() and SystemError() from documentation.
 *  Use only CHECK() and SYSTEM_CHECK().
 *
 *  Revision: 1.22 Thu Mar 22 19:21:39 2007 stephane.drouard@st.com
 *  Updated comments.
 *
 *  Revision: 1.21 Thu Mar 22 17:55:49 2007 stephane.drouard@st.com
 *  Added IMG_Crop().
 *
 *  Revision: 1.20 Tue Mar 20 18:11:52 2007 stephane.drouard@st.com
 *  Added IMG_ReplacePlane().
 *
 *  Revision: 1.19 Tue Mar 20 11:08:56 2007 stephane.drouard@st.com
 *  Added Bayer jogs support into an image.
 *
 *  Revision: 1.18 Mon Mar 19 18:06:07 2007 stephane.drouard@st.com
 *  Modified IMG_Create to pass image attributes.
 *
 *  Revision: 1.17 Wed Feb 21 15:35:04 2007 stephane.drouard@st.com
 *  Added IMG_TYPE_LITTLE_ENDIAN for IMG_Load().
 *
 *  Revision: 1.16 Tue Feb 20 10:28:14 2007 stephane.drouard@st.com
 *  Changed CMDLINE syntax.
 *
 *  Revision: 1.15 Fri Feb  9 12:31:56 2007 stephane.drouard@st.com
 *  Shortened color names.
 *
 *  Revision: 1.14 Fri Feb  9 11:49:04 2007 stephane.drouard@st.com
 *  Added IMG_ExtractColorPlane() and IMG_ExtractBayerPlane().
 *
 *  Revision: 1.13 Wed Jan 31 18:56:40 2007 stephane.drouard@st.com
 *  Modified VERBOSE macro: "\n" is no more automatically appended.
 *
 *  Revision: 1.12 Tue Nov 28 18:43:34 2006 stephane.drouard@st.com
 *  Updated comments.
 *  Updated error messages.
 *
 *  Revision: 1.11 Tue Nov 28 16:35:36 2006 stephane.drouard@st.com
 *  Added verbose messages (section and key not found).
 *
 *  Revision: 1.10 Tue Nov 28 15:35:11 2006 stephane.drouard@st.com
 *  Added VERBOSE() macro.
 *
 *  Revision: 1.9 Fri Nov 17 15:05:40 2006 stephane.drouard@st.com
 *  Added CFGFILE module.
 *  Added some string functions:
 *    strcasechr(),
 *    strcaserchr(),
 *    strcasestr(),
 *    TrimLeft(),
 *    TrimRight().
 *
 *  Revision: 1.8 Fri Nov 10 16:31:28 2006 stephane.drouard@st.com
 *  Added Linux support.
 *  Added some common include files.
 *
 *  Revision: 1.10 Wed Oct 25 18:08:31 2006 stephane.drouard@st.com
 *  Removed bool_t, FALSE and TRUE, replaced by the C99's standard one (bool, false and true).
 *  Added CHECK() and SYSTEM_CHECK() macros.
 *  Rewrote CMDLINE macros.
 *  Added IMG_MAX_WIDTH/HEIGHT.
 *
 *  Revision: 1.9 Mon Sep 11 11:46:36 2006 stephane.drouard@st.com
 *  Removed IMG_Compare() and IMG_Crop() (implemented into their respective tools).
 *
 *  Revision: 1.8 Mon Aug 28 16:06:07 2006 stephane.drouard@st.com
 *  Renamed CMDLINE_OPTION_PARAM as CMDLINE_OPTION_STRING
 *  Renamed CMDLINE_OPTION_PARAM_SCAN as CMDLINE_OPTION_VALUE
 *  Renamed CMDLINE_PARAM as CMDLINE_PARAM_STRING
 *  Renamed CMDLINE_PARAM_SCAN as CMDLINE_PARAM_VALUE
 *  Added IMG_FORMAT_DEFAULT
 *
 *  Revision: 1.7 Wed Aug 23 17:20:50 2006 stephane.drouard@st.com
 *  Corrected doc.
 *
 *  Revision: 1.6 Wed Aug 23 15:47:11 2006 olivier.lemarchand@st.com
 *  Just to be sure ;-)
 *
 *  Revision: 1.5 Wed Aug 23 15:43:43 2006 olivier.lemarchand@st.com
 *  Removed Aliases from comments
 *
 *  Revision: 1.4 Wed Aug 23 15:19:39 2006 stephane.drouard@st.com
 *  Added IMG_PIXEL()
 *  Updated doc
 *
 *  Revision: 1.3 Tue Aug 22 16:52:38 2006 olivier.lemarchand@st.com
 *  Added Aliases RCS keyword in header
 *
 *  Revision: 1.2 Tue Aug 22 11:21:50 2006 olivier.lemarchand@st.com
 *  Added Synchronicity header.
 ******************************************************************************/
#ifndef __INCLUDE_RAID_H__
#define __INCLUDE_RAID_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Compiler support.
#if defined(_MSC_VER)
  // Microsoft Visual C++.
  #if _MSC_VER < 1400
    #error This library requires Visual C++ 2005.
  #endif
  #if !defined(__cplusplus)
  typedef int bool;
  enum { false, true };
  #endif
  typedef signed char int8_t;
  typedef unsigned char uint8_t;
  typedef signed short int16_t;
  typedef unsigned short uint16_t;
  typedef signed int int32_t;
  typedef unsigned int uint32_t;
  typedef signed long long int64_t;
  typedef unsigned long long uint64_t;

  #pragma warning(disable: 4996) // "xxx" was declared deprecated
  #define strncasecmp  strnicmp
  #define snprintf  _snprintf
#else
  // Assumed to be gcc.
  #include <inttypes.h>
  #include <stdbool.h>
#endif

#if defined(SOFTIP)
  #ifndef BUILDING_SOFTIP
    #define BUILDING_SOFTIP
  #endif
  #include <viperDynalink.h>
#elif defined(NATHAIR)
  #include <Image.h>
  #include <Selection.h>
#elif !defined(STANDALONE)
//  #error You must define one of STANDALONE, SOFTIP or NATHAIR.
#endif

/**
 * @mainpage
 *
 * This library provides basic features to develop applications for RAID:
 *  - @ref UTILS,
 *  - @ref VERBOSE,
 *  - @ref ERROR,
 *  - @ref CMDLINE,
 *  - @ref IMG,
 *  - @ref CFGFILE.\n
 *
 * This library was first developed for standalone developments, but extended
 * afterwards to other environments.
 *
 * Some functions/macros are specific to environments, some others may behave
 * a bit differently.
 *
 * A macro must be defined at compile time to select the target environment:
 *  - @p STANDALONE for standalone environment,
 *  - @p SOFTIP for SoftIP environment,
 *  - @p NATHAIR for Nathair environment.
 *
 * This library can be compiled:
 *  - in C or C++ (C++ needed for Nathair environment),
 *  - with gcc/g++ under Linux, SunOS or Windows,
 *  - with Visual C++ 2005.
 */

/**
 * @defgroup UTILS Common types and utilities
 *
 * This module provides a few common types and utilities.
 *
 * To increase readability and portability, you should use the C99's provided
 * types:
 *  - @p bool, and its associated values @p true and @p false
 *  - @p int8_t
 *  - @p uint8_t
 *  - @p int16_t
 *  - @p uint16_t
 *  - @p int32_t
 *  - @p uint32_t
 * @{
 */

/**
 * Gets the number of elements of a static/local array.
 *
 * @b Example:
 * @code
 * static const char *levels[] = { "WARNING", "INFO", "DEBUG" };
 * assert(level >= 0 && level < countof(levels));
 * @endcode
 */
#define countof(a)  (sizeof(a) / sizeof((a)[0]))

/**
 * Absolute value.
 *
 * @param a             Value.
 * @return              Absolute value.
 */
#define ABS(a)  ((a) < 0 ? -(a) : (a))

/**
 * Minimum of 2 values.
 *
 * @param a             Value.
 * @param b             Value.
 * @return              Minimum value.
 */
#define MIN(a, b)  ((a) < (b) ? (a) : (b))

/**
 * Maximum of 2 values.
 *
 * @param a             Value.
 * @param b             Value.
 * @return              Maximum value.
 */
#define MAX(a, b)  ((a) > (b) ? (a) : (b))

/**
 * Clips a value between a range.
 *
 * @param a             Value to clip.
 * @param min           Minimum value.
 * @param max           Maximum value.
 * @return              Clipped value.
 */
#define CLIP(a, min, max) \
        ((a) > (max) ? (max) : (a) < (min) ? (min) : (a))

/**
 * Unsigned rounded integer division.
 *
 * @param n             Numerator.
 * @param d             Denominator.
 * @return              Division value.
 */
#define UDIV(n, d) (((n) + (d) / 2) / (d))

/**
 * Signed rounded integer division.
 *
 * @param n             Numerator.
 * @param d             Denominator.
 * @return              Division value.
 */
#define SDIV(n, d) (((n) + ((n) < 0 ? -(d) / 2 : (d) / 2)) / (d))

#if defined(STANDALONE)
  extern char __verbose;
  enum { __WARNING, __INFO, __DEBUG };
  void __Verbose(int level, const char *msg, ...);
  void __SetAppName(const char *argv0);
  const char *GetAppName(void);
#elif defined(SOFTIP)
  void *__TryRaidLib(const char *module);
  void __EndTryRaidLib(void);
#elif defined(NATHAIR)
  extern Logger __logger;
  void SetModuleName(const char *name);
  const char *GetModuleName(void);
  template <typename T> T FromSwig(PyObject *obj);
  void __CppExn2PyErr();
  #define NO_IMPORT_ARRAY
  #include <numpy/arrayobject.h>
  void* GetArrayBuffer(PyArrayObject* obj, int size);
#endif

/*char *strcasechr(const char *s, int c);
char *strcaserchr(const char *s, int c);
char *strcasestr(const char *s1, const char *s2);*/
char *TrimLeft(const char *s);
char *TrimRight(char *s);

void __Error(const char *file, int line, const char *msg, ...);
void __SystemError(const char *file, int line, const char *msg, ...);

/** @} */

/**
 * @defgroup VERBOSE Verbose modes
 *
 * This module provides a few macros to report internal information with several
 * verbosity levels.
 *  - @b Warning @b level \n
 *    Used to report something wrong, but that has been got round.
 *    @code
 *    if (clip >= 1 << image.bits) {
 *      WARNING("Given clipping value (%d) > max pixel value - set to %d",
 *        clip, (1 << image.bits) - 1);
 *      clip = (1 << image.bits) - 1;
 *    }
 *    @endcode
 *
 *  - @b Informative @b level \n
 *    Used to report main actions, without going into details.
 *    @code
 *    INFO("Loading %s...", filename);
 *    @endcode
 *
 *  - @b Debug @b level \n
 *    Used to report details.\n
 *    Note that this macro does nothing when compiled with @p NDEBUG defined.
 *    @code
 *    DEBUG("Output pixel value = %d", *out);
 *    @endcode
 *
 * \n
 * The behaviour of these macros depends on the environment.
 *  - @b Standalone @b environment \n
 *    The messages are printed on the standard error (a "\n" is appended at the
 *    end of the messages).\n
 *    \n
 *    Warning messages are always printed.\n
 *    Informative messages are printed in standard and advanced verbose modes.\n
 *    Debug messages are printed in advanced verbose mode only.\n
 *    \n
 *    See @ref CMDLINE to set verbosity level:
 *     - "-v" enables the standard verbose mode,
 *     - "-v -v" enables the advanced verbose mode.
 *
 *  - @b SoftIP @b environment \n
 *    These macros do nothing.
 *
 *  - @b Nathair @b environment \n
 *    The messages are sent to a Python logger using the same level as the macro
 *    names suggest.\n
 *    The name of the Python logger is the @ref SetModuleName "module" name.\n
 *    The logging level has to be set at Python side.
 * @{
 */

/**
 * Reports a warning message.
 *
 * @param msg           [const char*] Message in printf format.
 * @param ...           Message's variable arguments.
 */
#if defined(STANDALONE)
  #define WARNING(msg, ...)  __Verbose(__WARNING, msg , ##__VA_ARGS__)
#elif defined(NATHAIR)
  #define WARNING(msg, ...)  __logger.Warn(msg , ##__VA_ARGS__)
#else
  #define WARNING(msg, ...)
#endif

/**
 * Reports an informative message.
 *
 * @param msg           [const char*] Message in printf format.
 * @param ...           Message's variable arguments.
 */
#if defined(STANDALONE)
  #define INFO(msg, ...) \
          (__verbose ? __Verbose(__INFO, msg , ##__VA_ARGS__) : (void)0)
#elif defined(NATHAIR)
  #define INFO(msg, ...)  __logger.Info(msg , ##__VA_ARGS__)
#else
  #define INFO(msg, ...)
#endif

/**
 * Reports a debug message.
 *
 * @param msg           [const char*] Message in printf format.
 * @param ...           Message's variable arguments.
 *
 * @note This macro does nothing when compiled with @p NDEBUG defined.
 */
#if defined(STANDALONE) && !defined(NDEBUG)
  #define DEBUG(msg, ...) \
          (__verbose > 1 ? __Verbose(__DEBUG, msg , ##__VA_ARGS__) : (void)0)
#elif defined(NATHAIR) && !defined(NDEBUG)
  #define DEBUG(msg, ...)  __logger.Debug(msg , ##__VA_ARGS__)
#else
  #define DEBUG(msg, ...)
#endif

/** @} */

/**
 * @defgroup ERROR Error handling
 *
 * This module provides error handling basics.
 *
 * There are 2 kinds of error:
 *  - runtime errors: errors that are due to the execution environment, like a
 *    file not found, an invalid command line value.\n
 *    For this kind of errors, use one of the provided macros.
 *  - programming errors: errors that are due to programming mistakes, like an
 *    invalid function parameter value.\n
 *    For this kind of errors, use the ANSI @p assert() macro.\n
 *    Note that @p assert() does nothing when compiled with @p NDEBUG defined.
 *
 * The behaviour, to report runtime errors, depends on the environment.
 *
 * @par Standalone environment
 * The error message is displayed on the standard error, then the application
 * exists with a status code of 1.\n
 * A "\n" is appended at the end of the message.
 *
 * @par SoftIP environment
 * A simple try/catch mechanism is implemented to catch errors and report them
 * through the SoftIP dedicated functions.\n
 * See TRY_RAID_LIBRARY().
 *
 * @par Nathair environment
 * Errors are reported through C++ exceptions.
 * @{
 */

/**
 * Checks for a condition.
 *
 * If the condition is false, the application exits with an error message.
 *
 * @param cond          Condition to check.
 * @param msg           [const char*] Error message in printf format.
 * @param ...           Error message's variable arguments.
 *
 * @b Example:
 * @code
 * CHECK(value >= min && value <= max,
 *   "%d: Must be between %d and %d", value, min, max);
 * @endcode
 */
#define CHECK(cond, msg, ...) \
        ((cond) ? (void)0 : __Error(__FILE__, __LINE__, msg , ##__VA_ARGS__))

/**
 * Checks for a condition resulting of a system call.
 *
 * If the condition is false, the application exits with an error message.
 *
 * @param cond          Condition to check.
 * @param msg           [const char*] Error message in printf format to be
 *                      displayed in front of the system message.
 * @param ...           Error message's variable arguments.
 *
 * @b Example:
 * @code
 * fp = fopen(filename, "rb");
 * SYSTEM_CHECK(fp != NULL, filename);
 * @endcode
 */
#define SYSTEM_CHECK(cond, msg, ...) \
        ((cond) ? (void)0 : __SystemError(__FILE__, __LINE__, msg , \
                                          ##__VA_ARGS__))

#if defined(SOFTIP) || defined(DOXYGEN)

#include <setjmp.h>

/**
 * Simple try/catch mechanism for RAID library errors.
 *
 * @par Specific to the SoftIP environment.
 *
 * When an error is detected, a message is displayed into SoftIP, then the
 * program jumps to the catch section.\n
 * Nesting is not allowed.
 *
 * The catch section is mandatory, but can possibly be empty.
 *
 * @param module        [const char*] Module name, to be reported when an error
 *                      occurs.\n
 *                      Typically it's the @p MODULE_NAME variable declared in
 *                      SoftIP wrapper file (viper_<module>.c)
 *
 * To be typically used as follows:
 * @code
 * IMG_image_t input, output;
 * TRY_RAID_LIBRARY(MODULE_NAME) {
 *   IMG_CreateLinkToSoftIpImage(&input, pimgIn, IMG_LINK_IN);
 *   IMG_CreateLinkToSoftIpImage(&output, pimgOut, IMG_LINK_OUT);
 *   algo(&input, &output, ...);
 * }
 * CATCH_RAID_LIBRARY() {
 * }
 * IMG_Free(&input);
 * IMG_Free(&output);
 * @endcode
 *
 * @note IMG_Free(), and only this function, is protected against uninitialized
 *       IMG_image_t variables.
 */
#define TRY_RAID_LIBRARY(module) \
        if (setjmp(__TryRaidLib(module)) == 0) {

/**
 * See TRY_RAID_LIBRARY().
 */
#define CATCH_RAID_LIBRARY() \
          __EndTryRaidLib(); \
        } else

#endif // defined(SOFTIP)

/** @} */

#if defined(STANDALONE) || defined(DOXYGEN)

/**
 * @defgroup CMDLINE Command line parser
 *
 * @par Specific to the standalone environment.
 *
 * This module provides macros to ease command line parsing of options and
 * parameters, by:
 *  - hiding control structures,
 *  - handling "-v" / "-verbose" options to set the verbosity level:
 *     - not specified: not in verbose mode,
 *     - specified once: standard verbose mode,
 *     - specified twice: advanced verbose mode.
 *  - handling "-h" / "-help" options to display the application name,
 *    version, help and usage,
 *  - performing some checks,
 *  - providing error handling.
 *
 * @anchor CMDLINE_ERROR
 * When an error occurs (internally or explicitly by using CMDLINE_CHECK()),
 * the printed error message includes how to get help and usage.
 *
 * @b Example:
 * @code
 * int main(int argc, const char *argv[])
 * {
 *   const char *input = "-", *output = "-", *banner = NULL;
 *   int width = 0, height = 0;
 *   float ratio = 0;
 *   bool transparent = false;
 *   IMG_image_t inImage, outImage;
 *
 *   CMDLINE_START(argc, argv, "1.0",
 *     "This is a dummy tool, just for command line example.");
 *   CMDLINE_PARAM(input, "[input]",
 *     "File name of the input image.\n"
 *     "If omitted or \"-\", read from the standard input.");
 *   CMDLINE_PARAM(output, "[output]",
 *     "File name of the output image.\n"
 *     "If omitted or \"-\", write to the standard output.");
 *   CMDLINE_OPTION("banner", "<text>",
 *     "Text of the banner to be displayed.\n"
 *     "If omitted, no banner is generated.")
 *   {
 *     CMDLINE_OPTION_STRING(banner);
 *   }
 *   CMDLINE_OPTION("transparent", NULL,
 *     "Banner is written in transparent mode.")
 *   {
 *     transparent = true;
 *   }
 *   CMDLINE_OPTION("size", "<width> <height>",
 *     "Size of the output image.\n"
 *     "If omitted, the size if the same as the input image.")
 *   {
 *     CMDLINE_OPTION_VALUE(width, "%d");
 *     CMDLINE_CHECK(width > 0, "Width must be > 0");
 *     CMDLINE_OPTION_VALUE(height, "%d");
 *     CMDLINE_CHECK(height > 0, "Height must be > 0");
 *   }
 *   CMDLINE_OPTION("ratio", "<ratio>",
 *     "Size of the output image as a ratio of the input.\n"
 *     "If omitted, the size if the same as the input.\n"
 *     "-ratio and -size are mutually exclusive.")
 *   {
 *     CMDLINE_OPTION_VALUE(ratio, "%f");
 *     CMDLINE_CHECK(ratio > 0.0 && ratio <= 1.0,
 *       "Ratio must be in the range ]0,1]");
 *   }
 *   CMDLINE_END(0);
 *
 *   CMDLINE_CHECK(ratio == 0 || width == 0,
 *     "-ratio and -size are mutually exclusive");
 *
 *   IMG_Load(&inImage, input, IMG_TYPE_ANY);
 *
 *   // Compute output image size.
 *   if (ratio >= 0) {
 *     width = inImage.width * ratio;
 *     height = inImage.height * ratio;
 *   }
 *   else if (width == 0) {
 *     width = inImage.width;
 *     height = inImage.height;
 *   }
 *
 *   IMG_Create(&outImage, width, height, inImage.type, inImage.bits);
 *   ProcessImage(&inImage, &outImage, banner, transparent);
 *   IMG_Save(&outImage, output, IMG_FORMAT_DEFAULT);
 *
 *   return 0;
 * }
 * @endcode
 *
 * @p "example -help" displays:
 * @verbatim
 example - 1.0
 This is a dummy tool, just for command line example.
 Usage: example [options] <input> [output]
     [input]
         File name of the input image.
         If omitted or "-", read from the standard input.
     [output]
         File name of the output image.
         If omitted or "-", write to the standard output.
     -banner <text>
         Text of the banner to be displayed.
         If omitted, no banner is generated.
     -transparent
         Banner is written in transparent mode.
     -size <width> <height>
         Size of the output image.
         If omitted, the size if the same as the input image.
     -ratio <ratio>
         Size of the output image as a ratio of the input.
         If omitted, the size if the same as the input.
         -ratio and -size are mutually exclusive.
     -v, -verbose
         Displays verbose messages (standard error).
         Specifying it twice may display advanced information.
     -h, -help
         Displays this help. @endverbatim
 * @{
 */

extern const char *__CMDLINE_arg;
void __CMDLINE_Error(const char *file, int line, const char *msg, ...);
void __CMDLINE_Usage(int step, const char *name, const char *params,
  const char *help, ...);

/**
 * Starts the command line analysis.
 *
 * This macro must be called before any other CMDLINE_... macro.
 *
 * @param argc          [int] Number of arguments.
 * @param argv          [const char*] Arguments.
 * @param version       [const char*] Version of the application.
 * @param help          [const char*] Help message in printf format.
 * @param ...           Help message's variable arguments.
 */
#define CMDLINE_START(argc, argv, version, help, ...) \
  { int _CL_i, _CL_param = 0, _CL_helpStep = 0; \
    char _CL_dummy; \
    __SetAppName(argv[0]); \
    for (_CL_i = 1; _CL_i < argc; _CL_i++) { \
      int _CL_index = 0; \
      __CMDLINE_arg = argv[_CL_i]; \
      if (_CL_helpStep == 0 && (strcmp(__CMDLINE_arg, "-h") == 0 \
          || strcmp(__CMDLINE_arg, "-help") == 0)) { \
        printf("%s - " version "\n" help "\nUsage: %s [options]", \
          GetAppName() , ##__VA_ARGS__, GetAppName()); \
        _CL_helpStep = 1; \
        _CL_i--; \
        continue; \
      } \
      if (_CL_helpStep == 0 && (strcmp(__CMDLINE_arg, "-v") == 0 \
          || strcmp(__CMDLINE_arg, "-verbose") == 0)) { \
        __verbose++

/**
 * Retrieves a parameter and possibly opens a parameter section.
 *
 * Inside the section, any valid C code can be written including calls to
 * CMDLINE_CHECK().
 *
 * @b Example (no section):
 * @code
 * CMDLINE_PARAM(input, "<input>",
 *   "File name of the input file.");
 * @endcode
 *
 * @b Example (with a section):
 * @code
 * CMDLINE_PARAM(output, "<output>",
 *   "File name of the output file.")
 * {
 *    CMDLINE_CHECK(!IsSameFile(input, output),
 *      "Can't be the same as the input");
 * }
 * @endcode
 *
 * @note Parameters should precede options, to be printed on top of the usage.
 *
 * @param var           [const char*] The variable to be set to the
 *                      parameter value.\n
 *                      It's a pointer copy, not a buffer copy.
 * @param name          [const char*] Name of the parameter.\n
 *                      Enclose the name within "<...>" or "[...]" to indicate
 *                      whether it is mandatory or optional.
 * @param help          [const char*] Help message in printf format.
 * @param ...           Help message's variable arguments.
 */
#define CMDLINE_PARAM(var, name, help, ...) \
        continue; \
      } \
      if (_CL_helpStep > 0) \
        __CMDLINE_Usage(_CL_helpStep, name, NULL, help , ##__VA_ARGS__); \
      else if ((__CMDLINE_arg[0] != '-' || __CMDLINE_arg[1] == 0) \
          && _CL_param == _CL_index++) { \
        var = __CMDLINE_arg; \
        _CL_param++; \
        if (true)

/**
 * Opens an option section.
 *
 * Inside the section, any valid C code can be written including calls to
 * CMDLINE_OPTION_STRING(), CMDLINE_OPTION_VALUE() and CMDLINE_CHECK().
 *
 * @b Example:
 * @code
 * CMDLINE_OPTION("size", "<width> <height>",
 *   "Size of the output image.")
 * {
 *   CMDLINE_OPTION_VALUE(width, "%d");
 *   CMDLINE_CHECK(width > 0, "Width must be > 0");
 *   CMDLINE_OPTION_VALUE(height, "%d");
 *   CMDLINE_CHECK(height > 0, "Height must be > 0");
 * }
 * @endcode
 *
 * @param name          [const char*] Name of the option (without the "-"
 *                      prefix).
 * @param params        [const char*] Parameter(s) of the option.\n
 *                      May be @p NULL or "" if the option does not accept
 *                      parameters.\n
 *                      Enclose the parameter(s) within "<...>".
 * @param help          [const char*] Help message in printf format.
 * @param ...           Help message's variable arguments.
 *
 * @note If several option sections are declared with the same name, only the
 *       first one will be executed.
 */
#define CMDLINE_OPTION(name, params, help, ...) \
        continue; \
      } \
      if (_CL_helpStep > 0) \
        __CMDLINE_Usage(_CL_helpStep, "-" name, params, help , ##__VA_ARGS__); \
      else if (strcmp(__CMDLINE_arg, "-" name) == 0) { \
        if (true)

/**
 * Gets an option parameter as a string.
 *
 * Must be used within a CMDLINE_OPTION() section.
 *
 * An @ref CMDLINE_ERROR "error is raised" if there are no more parameters.
 *
 * @param var           [const char*] The variable to be set to the parameter
 *                      value.\n
 *                      It's a pointer copy, not a buffer copy.
 */
#define CMDLINE_OPTION_STRING(var) \
        CMDLINE_CHECK(++_CL_i < argc, "Syntax error"); \
        var = argv[_CL_i]

/**
 * Gets an option parameter as a value of any type.
 *
 * Must be used within a CMDLINE_OPTION() section.
 *
 * An @ref CMDLINE_ERROR "error is raised" if there are no more parameters, or
 * the parameter does not match the expected format.
 *
 * @param var           The variable to be set to the retrieved value.\n
 *                      Must be compatible with the type used in @p fmt.
 * @param fmt           [const char*] Type in scanf() format.\n
 *                      Do not prefix the variable with "&" (as with scanf()).
 */
#define CMDLINE_OPTION_VALUE(var, fmt) \
        CMDLINE_CHECK(++_CL_i < argc \
            && sscanf(argv[_CL_i], fmt "%c", &var, &_CL_dummy) == 1, \
          "Syntax error")

/**
 * Checks for a condition.
 *
 * An @ref CMDLINE_ERROR "error is raised" if the condition is false.
 *
 * This macro can be called within CMDLINE_PARAM() and CMDLINE_OPTION()
 * sections, for checks relative to the current parameter or option, or after
 * CMDLINE_END() as global checks.
 *
 * @param cond          Condition to check.
 * @param msg           [const char*] Error message in printf format.
 * @param ...           Error message's variable arguments.
 */
#define CMDLINE_CHECK(cond, msg, ...) \
        ((cond) ? (void)0 : __CMDLINE_Error(__FILE__, __LINE__, msg , \
                                            ##__VA_ARGS__))


#ifndef NDEBUG
  #define __CMDLINE_VERBOSE_HELP \
          "Displays verbose messages (standard error).\n" \
          "Specifying it twice may display advanced information."
#else
  #define __CMDLINE_VERBOSE_HELP \
          "Displays verbose messages (standard error)."
#endif

/**
 * Ends the command line analysis.
 *
 * This macro must be called after all CMDLINE_PARAM() and CMDLINE_OPTION()
 * sections.
 *
 * @param params        [int] Number of mandatory parameters.\n
 *                      An @ref CMDLINE_ERROR "error is raised" if the number
 *                      of retrieved parameters is less than this value.
 */
#define CMDLINE_END(params) \
        continue; \
      } \
      switch (_CL_helpStep) { \
      case 1: \
        putchar('\n'); \
        _CL_helpStep = 2; \
        _CL_i--; \
        continue; \
      case 2: \
        __CMDLINE_Usage(_CL_helpStep, "-v, -verbose", NULL, \
          __CMDLINE_VERBOSE_HELP); \
        __CMDLINE_Usage(_CL_helpStep, "-h, -help", NULL, \
          "Displays this help."); \
        exit(0); \
      } \
      __CMDLINE_Error(__FILE__, __LINE__, \
        *__CMDLINE_arg == '-' ? "Unknown option" : "Too many parameters"); \
      _CL_dummy = 0; /* To avoid a warning. */ \
    } \
    __CMDLINE_arg = NULL; \
    CMDLINE_CHECK(_CL_param >= params, "Too few parameters"); \
  }

/** @} */

#endif // defined(STANDALONE)

/**
 * @defgroup IMG Image support
 *
 * This module provides a basic support of image file management.
 *
 * Only PGM and PPM file formats are supported, in both binary and ASCII
 * formats.
 * @{
 */

/**
 * @defgroup IMG_TYPE Image types
 *
 * Defines the types of an image.
 * @{
 */

/** Grey scale (1 channel).
 *  This type also applies to Bayer images. */
#define IMG_TYPE_GREY   1
/** RGB (3 channels). */
#define IMG_TYPE_RGB    3
/** YCbCr (3 channels). */
#define IMG_TYPE_YCBCR  7
/** Specific to IMG_Load() and IMG_LoadHeader(), accepts any type of image. */
#define IMG_TYPE_ANY   0
/** Specific to IMG_Load(), swaps the image data.
 *  To be OR'ed with one of the other types. */
#define IMG_TYPE_LITTLE_ENDIAN  0x100

/** @} */

/**
 * @defgroup IMG_FORMAT Image file formats
 *
 * Defines the formats of an image file.
 * @{
 */

/** Default format. */
#define IMG_FORMAT_DEFAULT  IMG_FORMAT_PNM_BINARY
/** PGM/PPM binary. */
#define IMG_FORMAT_PNM_BINARY  0
/** PGM/PPM ASCII. */
#define IMG_FORMAT_PNM_ASCII   1

/** @} */

/**
 * @defgroup IMG_COLOR Color planes
 *
 * Defines the color planes of an image.
 * @{
 */

/** Red. */
#define IMG_COLOR_R   0
/** Green. */
#define IMG_COLOR_G   1
/** Blue. */
#define IMG_COLOR_B   2
/** Green in red (Bayer image). */
#define IMG_COLOR_GIR 3
/** Green in blue (Bayer image). */
#define IMG_COLOR_GIB 4
/** Specific to IMG_ExtractPlane(), merges both greens (Bayer image). */
#define IMG_COLOR_2G  5
/** Y. */
#define IMG_COLOR_Y   0
/** Cb. */
#define IMG_COLOR_CB  1
/** Cr. */
#define IMG_COLOR_CR  2

/** @} */

/**
 * @defgroup IMG_SHIFT Shift options
 *
 * Defines options for the shift operation (IMG_Shift()).
 * @{
 */

/** Copy MS bits into LS bits when left shifting.
 *  When this flag is not set, LB bits are let to 0. */
#define IMG_SHIFT_COPY_MS_BITS  0x1000

/** Round values when right shifting. */
#define IMG_SHIFT_ROUND         0x2000

/** @} */

#if defined(SOFTIP) || defined(DOXYGEN)

/**
 * @defgroup IMG_LINK_SOFTIP Link types to SoftIP images
 *
 * @par Specific to the SoftIP environment.
 *
 * Defines the types of link to SoftIP images.
 * @{
 */

/** Input only. */
#define IMG_LINK_IN     0
/** Input/output. */
#define IMG_LINK_INOUT  1
/** Output only. */
#define IMG_LINK_OUT    2

/** @} */

#endif // defined(SOFTIP)

#if defined(NATHAIR) || defined(DOXYGEN)

/**
 * @defgroup IMG_LINK_NATHAIR Link types to Nathair images
 *
 * @par Specific to the Nathair environment.
 *
 * Defines the types of link to Nathair images.
 * @{
 */

/** The linked image will not be modified. */
#define IMG_LINK_CONST  0x100

/** @} */

#endif // defined(NATHAIR)

/**
 * Color channel data type.
 */
typedef uint16_t IMG_data_t;

/**
 * Image attributes.
 *
 * @warning The structure members can be read, but should never be directly
 *          modified.
 */
typedef struct _IMG_image_t {
#if defined(__cplusplus)
   _IMG_image_t();
   ~_IMG_image_t();
#endif

  /** Image width in pixels. */
  int width;
  /** Image height in lines. */
  int height;
  /** @ref IMG_TYPE "Image type". */
  int type;
  /** Number of bits per color channel [1,16]. */
  int bits;
  /** Bayer jogs. */
  bool hJog, vJog;
  /** Color channel data storage. */
  IMG_data_t *data;

  #if defined(SOFTIP) || defined(DOXYGEN)
  /** @internal
   * Signature to validate structure contents.\n
   * Used by IMG_Free() to know whether the structure has been initialized
   * or not (see TRY_RAID_LIBRARY()). */
  int signature;
  /** Address of the SoftIP image in case of a link.
   *  @p NULL if not a link. */
  const TVImage *softIpImage;
  /** @ref IMG_LINK_SOFTIP "Type of link" to the SoftIP image. */
  int linkType;
  #endif
  #if defined(NATHAIR) || defined(DOXYGEN)
  /** Address of the Nathair image in case of a link.
   *  @p NULL if not a link. */
  Image *nathairImage;
  #endif
} IMG_image_t;

/**
 * Gets the number of pixels of an image.
 *
 * @param image         [IMG_image_t] Image.
 * @return              [int] Number of pixels.
 */
#define IMG_PIXELS(image)  ((image).height * (image).width)

/**
 * Gets the number of color channels of an image.
 *
 * @param image         [IMG_image_t] Image.
 * @return              [int] Number of channels {1,3}.
 */
#define IMG_CHANNELS(image)  ((image).type & 0x03)

/**
 * Gets the pixel offset from a (x,y) position of an image.
 *
 * @param image         [IMG_image_t] Image.
 * @param x             [int] X position.
 * @param y             [int] Y position.
 * @return              [int] Pixel offset.
 *
 * @note For RGB/YCbCr images, the offset corresponds to the R/Y value,
 *       offset + 1 the G/Cb value and offset + 2 the B/Cr value.
 */
#define IMG_OFFSET(image, x, y) \
        (((image).width * (y) + (x)) * IMG_CHANNELS(image))

/**
 * Gets the pixel address from a (x,y) position of an image.
 *
 * @param image         [IMG_image_t] Image.
 * @param x             [int] X position.
 * @param y             [int] Y position.
 * @return              [IMG_data_t*] Pixel address.
 *
 * @note For RGB/YCbCr images, address[0] corresponds to the R/Y value,
 *       address[1] the G/Cb value and address[2] the B/Cr value.
 */
#define IMG_PIXEL(image, x, y) \
        ((image).data + IMG_OFFSET(image, x, y))

/**
 * Gets the horizontal jog of a X position of a Bayer image.
 *
 * @param image         [IMG_image_t] Image.
 * @param x             [int] X position.
 * @return              [bool] Jog.
 *
 * @note X can be replaced by a horizontal jog, assuming its value is limited to
 *       0 (false) and 1 (true).
 */
#define IMG_HJOG(image, x) \
        ((image).hJog ^ ((x) & 1))

/**
 * Gets the vertical jog of a Y position of a Bayer image.
 *
 * @param image         [IMG_image_t] Image.
 * @param y             [int] Y position.
 * @return              [bool] Jog.
 *
 * @note Y can be replaced by a vertical jog, assuming its value is limited to
 *       0 (false) and 1 (true).
 */
#define IMG_VJOG(image, y) \
        ((image).vJog ^ ((y) & 1))

/**
 * Gets the color of a (x,y) position of a Bayer image.
 *
 * @param image         [IMG_image_t] Image.
 * @param x             [int] X position.
 * @param y             [int] Y position.
 * @return              [int] #IMG_COLOR_GIR, #IMG_COLOR_R, #IMG_COLOR_B or
 *                      #IMG_COLOR_GIB.
 *
 * @note X and Y can be replaced by respectively a horizontal jog and a vertical
 *       jog, assuming their values are limited to 0 (false) and 1 (true).
 */
#define IMG_BAYER_COLOR(image, x, y) \
        __IMG_bayerColors[IMG_VJOG(image, y)][IMG_HJOG(image, x)]

extern const char __IMG_bayerColors[2][2];

/**
 * Extracts a kernel around a central pixel.
 *
 * For RGB and YCbCr images, the kernel is extracted from a color plane.
 * The central pixel has to be offset by the expected color plane.
 * See IMG_PIXEL()'s note.
 *
 * @param image         [IMG_image_t] Image.
 * @param center        [IMG_data_t*] Pointer to the central pixel.
 * @param kernel        [IMG_data_t* or IMG_data_t**, out] 1D or 2D array to
 *                      store the kernel values.
 * @param width         [int] Kernel width.
 * @param height        [int] Kernel height.
 *
 * The mapping of the pixels depends on the array shape. Example with a 3x3
 * kernel:
 * - 1D array:\n
 *   @code
 *   kernel[0] kernel[1] kernel[2]
 *   kernel[3] kernel[4] kernel[5]
 *   kernel[6] kernel[7] kernel[8]
 *   @endcode
 * - 2D array:\n
 *   @code
 *   kernel[0][0] kernel[0][1] kernel[0][2]
 *   kernel[1][0] kernel[1][1] kernel[1][2]
 *   kernel[2][0] kernel[2][1] kernel[2][2]
 *   @endcode
 */
#define IMG_EXTRACT_KERNEL(image, center, kernel, width, height) \
        do { \
          int x, y; \
          IMG_data_t *k = (IMG_data_t*)kernel; \
          for (y = -(height) / 2; y <= (height) / 2; y++) \
            for (x = -(width) / 2; x <= (width) / 2; x++) \
              *k++ = center[IMG_OFFSET(image, x, y)]; \
        } while (0)

/**
 * Extracts a Bayer ring around a central pixel.
 *
 * @param image         [IMG_image_t] Image.
 * @param center        [IMG_data_t*] Pointer to the central pixel.
 * @param diamond       [bool] Get a diamond ring instead of a square ring.
 * @param ring          [IMG_data_t*, out] Array to store the ring values.
 *
 * The mapping of the pixels depends on the mode:
 * - square mode:\n
 *   @code
 *   ring[0]   -   ring[1]   -   ring[2]
 *      -      -      -      -      -
 *   ring[7]   -      -      -   ring[3]
 *      -      -      -      -      -
 *   ring[6]   -   ring[5]   -   ring[4]
 *   @endcode
 * - diamond mode:\n
 *   @code
 *      -      -   ring[1]   -      -
 *      -   ring[0]  -    ring[2]   -
 *   ring[7]  -      -       -   ring[3]
 *      -   ring[6]  -    ring[4]   -
 *      -      -   ring[5]   -      -
 *   @endcode
 *
 * @see IMG_PIXEL(), IMG_BAYER_COLOR().
 */
#define IMG_EXTRACT_BAYER_RING(image, center, diamond, ring) \
        do { \
          if (diamond) { \
            ring[0] = center[IMG_OFFSET(image, -1, -1)]; \
            ring[1] = center[IMG_OFFSET(image,  0, -2)]; \
            ring[2] = center[IMG_OFFSET(image,  1, -1)]; \
            ring[3] = center[IMG_OFFSET(image,  2,  0)]; \
            ring[4] = center[IMG_OFFSET(image,  1,  1)]; \
            ring[5] = center[IMG_OFFSET(image,  0,  2)]; \
            ring[6] = center[IMG_OFFSET(image, -1,  1)]; \
            ring[7] = center[IMG_OFFSET(image, -2,  0)]; \
          } else { \
            ring[0] = center[IMG_OFFSET(image, -2, -2)]; \
            ring[1] = center[IMG_OFFSET(image,  0, -2)]; \
            ring[2] = center[IMG_OFFSET(image,  2, -2)]; \
            ring[3] = center[IMG_OFFSET(image,  2,  0)]; \
            ring[4] = center[IMG_OFFSET(image,  2,  2)]; \
            ring[5] = center[IMG_OFFSET(image,  0,  2)]; \
            ring[6] = center[IMG_OFFSET(image, -2,  2)]; \
            ring[7] = center[IMG_OFFSET(image, -2,  0)]; \
          } \
        } while (0)

#if defined(NATHAIR) || defined(DOXYGEN)
/**
 * Retrieves the C++ Image object from a Nathair Python image object.
 *
 * @par Specific to the Nathair environment.
 *
 * @param obj           Python object.
 * @return              C++ object.
 *
 * @see FromSwig().
 */
inline Image *AsImage(PyObject *obj)
{
  return FromSwig<Image*>(obj);
}

/**
 * Retrieves the C++ Selection object from a Nathair Python selection object.
 *
 * @par Specific to the Nathair environment.
 *
 * @param obj           Python object.
 * @return              C++ object.
 *
 * @see FromSwig().
 */
inline Selection *AsSelection(PyObject *obj)
{
  return FromSwig<Selection*>(obj);
}
#endif // defined(NATHAIR)

int IMG_GetBits(int maxValue);
void IMG_Create(IMG_image_t *image, int width, int height, int type, int bits);
#if defined(SOFTIP)
void IMG_CreateLinkToSoftIpImage(IMG_image_t *image, const TVImage *softIpImage,
  int linkType);
#elif defined(NATHAIR)
void IMG_CreateLinkToNathairImage(IMG_image_t *image, Image *nathairImage,
  int type);
#endif
void IMG_Duplicate(const IMG_image_t *image1, IMG_image_t *image2);
void IMG_Free(IMG_image_t *image);
void IMG_LoadHeader(IMG_image_t *image, const char *filename, int type);
void IMG_Load(IMG_image_t *image, const char *filename, int type);
void IMG_Save(const IMG_image_t *image, const char *filename, int format);
void IMG_SetBayerJogs(IMG_image_t *image, bool hJog, bool vJog);
void IMG_Check(const IMG_image_t *image, const char *msg);
void IMG_Clip(IMG_image_t *image, int maxMsg, const char *msg);
void IMG_Mask(IMG_image_t *image, int maxMsg, const char *msg);
bool IMG_Shift(IMG_image_t *image, int bits);
void IMG_Crop(IMG_image_t *image, int left, int top, int right, int bottom);
void IMG_Copy(const IMG_image_t *image1, int x1, int y1, int width, int height,
  IMG_image_t *image2, int x2, int y2);
void IMG_CopyBorder(const IMG_image_t *image1, int width, int height,
  IMG_image_t *image2);
void IMG_GetBayerColorPosition(const IMG_image_t *image, int color,
  int *x, int *y);
void IMG_ExtractPlane(const IMG_image_t *image, int color, IMG_image_t *plane);
void IMG_ReplacePlane(IMG_image_t *image, int color, const IMG_image_t *plane);

#if defined(__cplusplus)
#include <algorithm>
inline void IMG_SortPixels(IMG_data_t *pixels, int n)
{
  std::sort(pixels, pixels + n);
}
#else
void IMG_SortPixels(IMG_data_t *pixels, int n);
#endif

/** @} */

/**
 * @defgroup CFGFILE Configuration file support
 *
 * This module provides a basic support of configuration files in INI format.
 *
 * Such a file can contain several sections, each section several keys
 * associated to a value.
 *
 * @b Example:
 * @verbatim
 [Section1]
 Key1=value
 Key2=value

 [Section2]
 Key1=value
 Key2=value @endverbatim
 *
 * Empty lines are allowed anywhere.\n
 * Lines starting with ';' are comments.\n
 * Section and key names are considered case-insensitive.
 * A same key name can be used in different sections.
 *
 * All sections and keys within a section should have a distinct name. If not,
 * only the first ones will be taken into account.
 * @{
 */

/**
 * Configuration file.
 */
typedef struct _CFGFILE_file_t {
#if defined(__cplusplus)
   _CFGFILE_file_t();
   ~_CFGFILE_file_t();
#endif

  /** File name. */
  char *filename;
  /** File content. */
  char *data;
} CFGFILE_file_t;

void CFGFILE_Load(CFGFILE_file_t *cfg, const char *filename);
void CFGFILE_Free(CFGFILE_file_t *cfg);
char *CFGFILE_GetNextSection(const CFGFILE_file_t *cfg, const char *section,
  char *value, int size);
char *CFGFILE_GetValue(const CFGFILE_file_t *cfg, const char *section,
  const char *key, char *value, int size, const char *defaultValue);

/** @} */

#if defined(NATHAIR) || defined(DOXYGEN)

/**
 * @addtogroup UTILS
 * @{
 */

/**
 * Retrieves the C++ object from a SWIG Python object.
 *
 * @par Specific to the Nathair environment.
 *
 * @tparam T            C++ object type.
 * @param obj           Python object.
 * @return              C++ object.
 *
 * @warning The Python object must not be deleted while the C++ object is used.
 */
template <typename T>
T FromSwig(PyObject *obj)
{
  PyObject *attr = PyObject_GetAttrString(obj, "this");
  CHECK(attr != NULL, "Not a SWIG object");
  unsigned int addr = PyInt_AsUnsignedLongMask(attr);
  Py_DECREF(attr);
  T ptr = reinterpret_cast<T>(addr);
  CHECK(dynamic_cast<T>(ptr) != NULL, "Not a SWIG object for this C++ type");
  return ptr;
}

/** @} */

#endif // defined(NATHAIR)

#if defined(NATHAIR) || defined(DOXYGEN)

/**
 * @defgroup TEMPLATE Template programming support
 *
 * @par Specific to the Nathair environment.
 *
 * This module provides the support of template classes to implement processing
 * for different data types.
 * @{
 */

/**
 * @defgroup TMPL_TYPE Type information class
 *
 * This class provides information for a given data type.
 * @{
 */

template <typename T>
class Type
{
};

template <>
class Type<short>
{
public:
  enum { Floating = false };
  typedef int Extended;
};

template <>
class Type<int>
{
public:
  enum { Floating = false };
  typedef long long Extended;
};

template <>
class Type<long long>
{
public:
  enum { Floating = false };
  // No extended type
};

template <>
class Type<float>
{
public:
  enum { Floating = true };
  typedef double Extended;
};

template <>
class Type<double>
{
public:
  enum { Floating = true };
  typedef double Extended;
};

/** @} */

/**
 * Division modes.
 * Only applies when numerator is integer.
 * A float division is always performed when numerator is floating.
 */
enum DivMode {
  eStdRounding = 0,      /// Uses a rounding similar to floating point.
  eNoRounding = 1,       /// Does not perform rounding.
  ePositiveRounding = 2, /// Performs a positive rounding even on negative values.
  eUseShift = 4          /// Div2() only: performs a shift instead of a division.
};

/**
 * Computes a division.
 *
 * @tparam T            Type of the numerator and result.
 * @tparam Td           Type of the divider.
 * @param num           Numerator.
 * @param div           Divider.
 * @param mode          See @ref DivMode.
 * @return              Result.
 */
template <typename T, typename Td>
T Div(T num, Td div, int mode = eStdRounding)
{
  assert(mode == eStdRounding || mode == eNoRounding || mode == ePositiveRounding);

  if (Type<T>::Floating)
    return T(num / div);
  if (mode == eNoRounding)
    return T(num / div);
  if (mode == eStdRounding && ((num < 0) ^ (div < 0)))
    return T((num - div / 2) / div);
  return T((num + div / 2) / div);
}

/**
 * Computes a division by a power of 2.
 *
 * @tparam T            Type of the numerator and result.
 * @param num           Numerator.
 * @param exp           Exponent.
 * @param mode          See @ref DivMode.
 * @return              Result.
 */
template<typename T>
T Div2(T num, int exp, int mode = eStdRounding)
{
  assert((mode & ~eUseShift) == eStdRounding || (mode & ~eUseShift) == eNoRounding || (mode & ~eUseShift) == ePositiveRounding);

  if ((mode & eUseShift) == 0)
    return Div(num, 1 << exp, mode);
  if (mode == (eUseShift | eNoRounding))
    return num >> exp;
  if (mode == (eUseShift | eStdRounding) && num < 0)
    return (num - (1 << (exp - 1))) >> exp;
  return (num + (1 << (exp - 1))) >> exp;
}

// Specialization for float.
inline float Div2(float num, int exp, int mode = eStdRounding)
{
  assert((mode & ~eUseShift) == eStdRounding || (mode & ~eUseShift) == eNoRounding || (mode & ~eUseShift) == ePositiveRounding);
  return Div(num, 1 << exp);
}
//
//// Specialization for double.
inline double Div2(double num, int exp, int mode = eStdRounding)
{
  assert((mode & ~eUseShift) == eStdRounding || (mode & ~eUseShift) == eNoRounding || (mode & ~eUseShift) == ePositiveRounding);
  return Div(num, 1 << exp);
}


//~ template <typename Tn, typename Td>
//~ Tn rdiv(Tn num, Td div)
//~ {
  //~ if (Type<Tn>::Floating)
    //~ return Tn(num / div);
  //~ else if ((num < 0) ^ (div < 0))
    //~ return Tn((num - div / 2) / div);
  //~ else
    //~ return Tn((num + div / 2) / div);
//~ }

//~ float rshift(float num, int bits) {
   //~ return float(num / (1<<bits));
//~ }

//~ double rshift(double num, int bits) {
   //~ return double(num / (1<<bits));
//~ }

//~ template <typename Tn>
//~ Tn rshift(Tn num, int bits)
//~ {
   //~ return Tn((num + (1 << (bits-1))) >> bits);
//~ }

//~ float shift(float num, int bits) {
   //~ return float(num / (1<<bits));
//~ }

//~ double shift(double num, int bits) {
   //~ return double(num / (1<<bits));
//~ }

//~ template <typename Tn>
//~ Tn shift(Tn num, int bits)
//~ {
   //~ return Tn(num >> bits);
//~ }

/**
 * Casts a value from one type to another type.
 *
 * A round is performed when the input value is floating and the output is
 * integer.
 *
 * @tparam To           Type of the output result.
 * @tparam Ti           Type of the input value.
 * @param result        Output result.
 * @param value         Input value.
 */
//template<typename To, typename Ti>
//void rcast(To& result, Ti value)
//{
// if (Type<Ti>::Floating && !Type<To>::Floating)
//    result = To(round(value));
//  else
//    result = To(value);
//}

// C++ compliant version
template<typename To, typename Ti>
To rounded_cast(Ti value)
{
  if (Type<Ti>::Floating && !Type<To>::Floating)
    return To(round(value));
  else
    return To(value);
}
/**
 * Sort n pixels values.
 *
 * @param pixels        Adress of the first pixel to sort.
 * @param n           	Number of pixels to sort.
 * @return            	void.
 */

template <typename T>
void SortPixels(T* pixels, int n)
{
  std::sort(pixels, pixels + n);
}



////////////////////////////////////////////////////////////////////////////////
// Image template class.

/**
 * Image template class.
 *
 * Wraps a Nathair image to a template class based on the pixel data type.
 *
 * @tparam T            Pixel data type.
 */
template <typename T>
class ImageT
{
protected:
  Image* _image;
  bool   lockImage;

  // Disallow copy.
  ImageT(const ImageT<T>&)
  {}

  // Disallow assignment.
  Image& operator=(const ImageT<T>&)
  {
    return *this;
  }

  string GetLockID() const
  {
    char s[200];
    snprintf(s, sizeof(s) - 1, "%s-%x", GetModuleName(), (int)this);
    return s;
  }

public:
  ImageT(const Image* image = NULL, bool lock=true)
  {

    lockImage = lock;
    if (image!=NULL)
      image->CheckBufferType(T(0));
    _image = (Image*)image;
  }

  ~ImageT()
  {
    if (_image!=NULL && lockImage) {
      const string lockID = GetLockID();
      if (_image->GetLockStatus(lockID))
        _image->Unlock(lockID);
    }
  }

  ImageT& operator=(const Image* image)
  {
    image->CheckBufferType(T(0));
    if (_image!=NULL && lockImage) {
      // Unlock current image if locked.
      string lockID = GetLockID();
      if (_image->GetLockStatus(lockID))
        _image->Unlock(lockID);
    }
    _image = (Image*)image;
    return *this;
  }

  const Image* GetImage() const
  {
    return _image;
  }

  void SetImage(Image* image)
  {
    _image = image;
  }

  unsigned int GetWidth() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetWidth();
  }

  unsigned int GetHeight() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetHeight();
  }

  unsigned int GetPlaneCount() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetPlaneCount();
  }

  unsigned int GetChannelCount() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetChannelCount();
  }

  string GetFilename() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetFilename();
  }

  void SetFilename(string filename)
  {
    CHECK(_image!=NULL, "image not available");
    _image->SetFilename(filename);
  }

  string GetLabel() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetLabel();
  }

  void SetLabel(string label)
  {
    CHECK(_image!=NULL, "image not available");
    _image->SetLabel(label);
  }

  string GetName() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetName();
  }

  Image::BufferType GetBufferType() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetBufferType();

  }


  const T* GetData() const
  {
    CHECK(_image!=NULL, "image not available");
    if (_image->GetLockStatus(GetLockID())) {
      _image->Unlock(GetLockID());
    }
    return (T*)_image->GetConstDataPtr(GetLockID());
  }

  T* GetData()
  {
    CHECK(_image!=NULL, "image not available");
    if (_image->GetLockStatus(GetLockID())) {
      _image->Unlock(GetLockID());
    }
    return (T*)_image->GetDataPtr(GetLockID());
  }

  unsigned int GetValueCount() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetValueCount();
  }

  unsigned int GetPixelCount() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetPixelCount();
  }

  string GetPixelInfo(unsigned int x, unsigned int y) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetPixelInfo(x, y);
  }

  //~ bool Lock(string requestorID) const;
  //~ void Unlock(string requestorID) const;
  //~ bool GetLockStatus(string requestorID) const;
  //~ bool GetLockStatus() const;
  //~ string GetNonConstLockOwner() const;

  string GetInfo() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetInfo();
  }

  unsigned int GetOrientation() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetOrientation();
  }

  void SetOrientation(unsigned int orientation)
  {
    CHECK(_image!=NULL, "image not available");
    _image->SetOrientation(orientation);
  }

  void NormaliseOrientation()
  {
    CHECK(_image!=NULL, "image not available");
    _image->NormaliseOrientation();
  }

  void HorizontalMirror()
  {
    CHECK(_image!=NULL, "image not available");
    _image->HorizontalMirror();
  }

  void VerticalFlip()
  {
    CHECK(_image!=NULL, "image not available");
    _image->VerticalFlip();
  }

  void Rotate(int degrees)
  {
    CHECK(_image!=NULL, "image not available");
    _image->Rotate(degrees);
  }

  bool IsValidImage() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->IsValidImage();
  }

  void EnsureValidImage(const string details, const string source = "") const
  {
    CHECK(_image!=NULL, "image not available");
    _image->EnsureValidImage(details, source);
  }

  void EnsureDataType(Image::DataType dataType, const string source = "") const
  {
    CHECK(_image!=NULL, "image not available");
    _image->EnsureDataType(dataType, source);
  }

  void EnsureIsChannelTypeOf(Image::ChannelType channelType, const string source = "") const
  {
    CHECK(_image!=NULL, "image not available");
    _image->EnsureIsChannelTypeOf(channelType, source);
  }

  void EnsureValidPixelCoords(unsigned int x, unsigned int y, const string description = "", const string source = "") const
  {
    CHECK(_image!=NULL, "image not available");
    _image->EnsureValidPixelCoords(x, y, description, source);
  }

  Image::DataType GetDataType() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetDataType();
  }

  void SetDataType(Image::DataType dataType)
  {
    CHECK(_image!=NULL, "image not available");
    _image->SetDataType(dataType);
    assert(_image->CheckBufferType(T(0)));
  }

  Image::ChannelType GetChannelType(unsigned int index = 0)
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetChannelType(index);
  }

  vector<Image::ChannelType> GetChannelTypesVector() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetChannelTypesVector();
  }

  void SetChannelType(Image::ChannelType channelType)
  {
    CHECK(_image!=NULL, "image not available");
    _image->SetChannelType(channelType);
  }

  void SetCustomChannelType(const string name, const string label)
  {
    CHECK(_image!=NULL, "image not available");
    _image->SetCustomChannelType(name, label);
  }

  string GetChannelTypeName(Image::ChannelType channelType) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetChannelTypeName(channelType);
  }

  string GetChannelTypeLabel(Image::ChannelType channelType) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetChannelTypeLabel(channelType);
  }

  string GetCustomChannelTypeName()
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetCustomChannelTypeName();
  }

  string GetCustomChannelTypeLabel()
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetCustomChannelTypeLabel();
  }

  Image::ChannelType GetBayerChannelType(unsigned int x, unsigned int y) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetBayerChannelType(x, y);
  }

  unsigned int GetBitsPerValue() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetBitsPerValue();
  }

  // Invalid as an ImageT, should be fixed according to trac #1368
  //~ void SetBitsPerValue(unsigned int bits)
  //~ {
    //~ //TODO bufferType=DenyBufferChange
    //~ _image->SetBitsPerValue(bits);
    //~ assert(_image->CheckBufferType(T(0)));
  //~ }

  double get_nominal_black() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->get_nominal_black();
  }

  double get_nominal_white() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->get_nominal_white();
  }

  std::vector<double> get_nominal_range() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->get_nominal_range();
  }

  void set_nominal_black(double black)
  {
    CHECK(_image!=NULL, "image not available");
    _image->set_nominal_black(black);
  }

  void set_nominal_white(double white)
  {
    CHECK(_image!=NULL, "image not available");
    _image->set_nominal_white(white);
  }

  void set_nominal_range(double black, double white)
  {
    CHECK(_image!=NULL, "image not available");
    _image->set_nominal_range(black, white);
  }

  // Underlying GetMinCode() and GetMaxCode() are unsigned which is incorrect in case of a signed or float image.
  // get_nominal_black() and get_nominal_white() could be preferred.
  // see trac #1368 for status
  //~ T GetMinCode() const
  //~ {
    //~ //TODO unsigned int
    //~ return _image->GetMinCode();
  //~ }

  //~ T GetMaxCode() const
  //~ {
    //~ //TODO unsigned int
    //~ return _image->GetMaxCode();
  //~ }

  void CopyFromBuffer(int width, int height, unsigned int bitsPerPlane, Image::DataType dataType, const T* buffer)
  {
    CHECK(_image!=NULL, "image not available");
    _image->CopyFromBuffer(width, height, bitsPerPlane, dataType, buffer);
  }

  void CopyFromBuffer(int width, int height, unsigned int bitsPerPlane, Image::DataType dataType, unsigned int bytesPerValue, const unsigned char* buffer)
  {
    CHECK(_image!=NULL, "image not available");
    _image->CopyFromBuffer(width, height, bitsPerPlane, dataType, bytesPerValue, buffer, Image::DenyBufferChange);
  }

  void Copy(const Image& other)
  {
    CHECK(_image!=NULL, "image not available");
    other.CheckBufferType(T(0)); // will become useless if denybufferchange is implemented on Copy() method
    _image->Copy(other);
  }

  void ShiftDataTo(unsigned int bitsPerValue, bool replicate_bits = true, bool round = true)
  {
    CHECK(_image!=NULL, "image not available");
    _image->ShiftDataTo(bitsPerValue, replicate_bits, round, Image::DenyBufferChange);
  }

  Image::ChannelType GetFirstBayerChannelType() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->GetFirstBayerChannelType();
  }

  void SetFirstBayerChannelType(Image::ChannelType channelType)
  {
    CHECK(_image!=NULL, "image not available");
    _image->SetFirstBayerChannelType(channelType);
  }

  //TODO To be integrated in nx.Image  (trac #1368)
  bool GetHorizontalJog(Image::ChannelType ref = Image::GreenOnRed) const
  {
    CHECK(_image!=NULL, "image not available");
    return Image::GetHorizontalJogBetween(_image->GetFirstBayerChannelType(), ref);
  }

  //TODO To be integrated in nx.Image (trac #1368)
  bool GetVerticalJog(Image::ChannelType ref = Image::GreenOnRed) const
  {
    CHECK(_image!=NULL, "image not available");
    return Image::GetVerticalJogBetween(_image->GetFirstBayerChannelType(), ref);
  }

  void switchEndianness()
  {
    CHECK(_image!=NULL, "image not available");
    _image->switchEndianness();
  }

  void CopyAllExceptDataValues(const Image& rhs)
  {
    CHECK(_image!=NULL, "image not available");
    _image->CopyAllExceptDataValues(rhs, Image::DenyBufferChange);
  }

  void CopyAllExceptDataValuesAndShape(const Image& rhs)
  {
    CHECK(_image!=NULL, "image not available");
    rhs.CheckBufferType(T(0));
    _image->CopyAllExceptDataValuesAndShape(rhs);
  }

  void Resize(unsigned int width, unsigned int height, Image::DataType dataType)
  {
    CHECK(_image!=NULL, "image not available");
    //TODO Need to add bitsPerValue for Python and DenyBufferChange for C++ parameters (see trac #1368)
    _image->Resize(width, height, dataType);
  }

  void Crop(unsigned int left, unsigned int top, unsigned int width, unsigned int height)
  {
    CHECK(_image!=NULL, "image not available");
    _image->Crop(left, top, width, height);
  }

  void CropBorders(unsigned int top, unsigned int right, unsigned int bottom, unsigned int left)
  {
    CHECK(_image!=NULL, "image not available");
    _image->CropBorders(top, right, bottom, left);
  }

  void CropBorders(unsigned int border)
  {
    CHECK(_image!=NULL, "image not available");
    _image->CropBorders(border);
  }

  Image ExtractChannel(Image::ChannelType channel) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->ExtractChannel(channel);
  }

  vector<Image> ExtractChannels() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->ExtractChannels();
  }

  map<Image::ChannelType, Image> ExtractChannelMap() const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->ExtractChannelMap();
  }

  Image astype(Image::BufferType type) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->astype(type);
  }

  // Disabled until deny buffer change is implemented at nx level (see trac #1368)
  //~ void Load(string filename, bool bigEndian = true)
  //~ {
    //~ //TODO Need to deny buffer change
    //~ _image->Load(filename, bigEndian);
  //~ }

  //~ void Load(string filename, int width, int height, Image::DataType dataType, int bytesPerValue, int bitsPerValue, bool bigEndian, int least_significant_bit)
  //~ {
    //~ //TODO Need to deny buffer change.
    //~ _image->Load(filename, width, height, dataType, bytesPerValue, bitsPerValue, bigEndian, least_significant_bit);
  //~ }

  void Save(string filename, bool bigEndian = true, bool binary = true) const
  {
    CHECK(_image!=NULL, "image not available");
    _image->Save(filename, bigEndian, binary);
  }

  void setMetadata(const string& key, bool value, bool persists = true)
  {
    CHECK(_image!=NULL, "image not available");
    _image->setMetadata(key, value, persists);
  }

  void setMetadata(const string& key, float value, bool persists = true)
  {
    CHECK(_image!=NULL, "image not available");
    _image->setMetadata(key, value, persists);
  }

  void setMetadata(const string& key, int value, bool persists = true)
  {
    CHECK(_image!=NULL, "image not available");
    _image->setMetadata(key, value, persists);
  }

  void setMetadata(const string& key, string value, bool persists = true)
  {
    CHECK(_image!=NULL, "image not available");
    _image->setMetadata(key, value, persists);
  }

  void setMetadata(const string& key, const vector<unsigned short>& value, bool persists = true)
  {
    CHECK(_image!=NULL, "image not available");
    _image->setMetadata(key, value, persists);
  }

  void removeMetadata(const string& key)
  {
    CHECK(_image!=NULL, "image not available");
    _image->removeMetadata(key);
  }

  void clearMetadata()
  {
    CHECK(_image!=NULL, "image not available");
    _image->clearMetadata();
  }

  int getIntMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->getIntMetadata(key);
  }

  float getFloatMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->getFloatMetadata(key);
  }

  string getStringMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->getStringMetadata(key);
  }

  bool getBoolMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->getBoolMetadata(key);
  }

  vector<unsigned short> getRawDataMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->getRawDataMetadata(key);
  }

  bool isIntMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->isIntMetadata(key);
  }

  bool isFloatMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->isFloatMetadata(key);
  }

  bool isStringMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->isStringMetadata(key);
  }

  bool isBoolMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->isBoolMetadata(key);
  }

  bool isRawDataMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->isRawDataMetadata(key);
  }

  bool containsMetadata(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->containsMetadata(key);
  }

  int metadataCount(bool onlyPersistentData = false) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->metadataCount(onlyPersistentData);
  }

  bool metadataPersists(const string& key) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->metadataPersists(key);
  }

  void setMetadataPersists(const string& key, bool persists = true)
  {
    CHECK(_image!=NULL, "image not available");
    _image->setMetadataPersists(key, persists);
  }

  list<string> metadataKeys(bool persistentOnly = false) const
  {
    CHECK(_image!=NULL, "image not available");
    return _image->metadataKeys(persistentOnly);
  }

  /**
   * get the pixel(x,y)'s adress.
   *
   * @tparam T            Type of the numerator and result.
   * @param x,y           pixel's coordinates.
   * @return              Result.
   */
  T* GetPixelPtr(int x, int y) const
  {
    CHECK(_image!=NULL, "image not available");
    return (T*)this->GetData() + ((_image)->GetWidth() * (y) + (x)) * _image->GetPlaneCount();
  }

  T* GetPixelPtr(int x, int y) {
    CHECK(_image!=NULL, "image not available");
    return (T*)this->GetData() + ((_image)->GetWidth() * (y) + (x)) * _image->GetPlaneCount();
  }
  /**
   * get the pixel(x,y)'s relative adress regarding the first pixel adress.
   *
   * @tparam T            Type of the numerator and result.
   * @param x,y           pixel's coordinates.
   * @return              Result.
   */
  int GetPixelOffset(int  x,int y) const  {
    CHECK(_image!=NULL, "image not available");
    return ((_image)->GetWidth() * (y) + (x)) * _image->GetPlaneCount();
  }
  /**
    * get the pixel(x,y)'s relative adress regarding the first pixel adress.
    *
    * @param width         _image's width in the border are copied.
    * @param height        _image's height in the border are copied.
    * @param image2        Border of _image are copied on image2.
    */
  void CopyBorder( int width, int height,ImageT<T>& image2)const
  {
    CHECK(_image!=NULL, "image not available");
    assert((*this).IsValidImage());
    assert(image2.IsValidImage());

    assert(width >= 0);
    assert(height >= 0);
    width = MIN(width, (int)(_image->GetWidth() / 2));
    height = MIN(height, (int)(_image->GetHeight() / 2));
    (*this).CopyData( 0, 0,
             _image->GetWidth(), height,
             image2, 0, 0);
    (*this).CopyData( 0, height,
             width, _image->GetHeight() - 2 * height,
             image2, 0, height);
    (*this).CopyData( _image->GetWidth() - width, height,
             width, _image->GetHeight() - 2 * height,
             image2, _image->GetWidth() - width, height);

    (*this).CopyData( 0, _image->GetHeight() - height,
             _image->GetWidth(), height,
             image2, 0,_image->GetHeight()-height);
  }
  /**
    * copy part of _image in image2.
    *
    * @tparam T            Type of the numerator and result.
    * @param x1,y1         pixel's coordinates of where the part to copy begin.
    * @param width,height  shape of the _image part to copy
    * @param x2,y2         pixel's coordinates of where the data are copied.
    */
  void CopyData(int x1, int y1, int width, int height,ImageT<T>& image2, int x2, int y2)const
  {
    CHECK(_image!=NULL, "image not available");
    int i, srcLen, dstLen, lineLen;
    assert(this->IsValidImage());
    assert(image2.IsValidImage());

    assert(x1 >= 0);
    assert(y1 >= 0);
    assert(x2 >= 0);
    assert(y2 >= 0);

    if (width <= 0 || height <= 0)
      return;

    CHECK(_image->GetDataType() == image2.GetDataType(),
      "Both images must be of the same type");
    CHECK(_image->GetBitsPerValue() == image2.GetBitsPerValue(),
      "Both images must be of the same bit range");
    CHECK(x1 + width <= (int)_image->GetWidth() && y1 + height <= (int)_image->GetHeight(),
      "Rectangle outside the source image");
    CHECK(x2 + width <= (int)image2.GetWidth() && y2 + height <= (int)image2.GetHeight(),
      "Rectangle outside the destination image");

    T* src = this->GetPixelPtr(x1, y1);
    T* dst = image2.GetPixelPtr( x2, y2);
    srcLen = _image->GetWidth() * _image->GetPlaneCount();
    dstLen = image2.GetWidth() * image2.GetPlaneCount();
    lineLen = width * _image->GetPlaneCount() * sizeof(T);
    if ((T*)this->GetData() == image2.GetData()) {
      if (y2 > y1) {
        src += (width - 1) * srcLen;
        dst += (width - 1) * dstLen;
        srcLen = -srcLen;
        dstLen = -dstLen;
      }
      for (i = 0; i < height; i++, src += srcLen, dst += dstLen)
        memmove(dst, src, lineLen);
    }
    else
    {
  	for (i = 0; i < height; i++, src += srcLen, dst += dstLen)
  	{

      	    memcpy(dst, src, lineLen);
  	}
    }
    if (image2.SeemsBayer() && x2 == 0 && y2 == 0)
  	  {image2.SetFirstBayerChannelType(_image->GetFirstBayerChannelType());}

  }
  bool SeemsBayer()const
  {
    CHECK(_image!=NULL, "image not available");
    if ((*this).IsValidImage())
      return _image->GetDataType()  == Image::Bayer;
    return false;

  }

  /**
    * ExtractBayerRing of the pixel center.
    *
    * @param center        pixel of which the ring is extracted .
    * @param diamond       form of the ring.
    * @param ring.         ring of pixel that is extracted from center.
    */

  void ExtractBayerRing(T* center,bool diamond, T* ring)const {
    CHECK(_image!=NULL, "image not available");
    if (diamond) {
      ring[0] = center[(*this).GetPixelOffset(-1, -1)];
      ring[1] = center[(*this).GetPixelOffset( 0, -2)];
      ring[2] = center[(*this).GetPixelOffset( 1, -1)];
      ring[3] = center[(*this).GetPixelOffset( 2,  0)];
      ring[4] = center[(*this).GetPixelOffset( 1,  1)];
      ring[5] = center[(*this).GetPixelOffset( 0,  2)];
      ring[6] = center[(*this).GetPixelOffset(-1,  1)];
      ring[7] = center[(*this).GetPixelOffset( -2,  0)];
    } else {
      ring[0] = center[(*this).GetPixelOffset( -2, -2)];
      ring[1] = center[(*this).GetPixelOffset(  0, -2)];
      ring[2] = center[(*this).GetPixelOffset(  2, -2)];
      ring[3] = center[(*this).GetPixelOffset( 2,  0)];
      ring[4] = center[(*this).GetPixelOffset( 2,  2)];
      ring[5] = center[(*this).GetPixelOffset( 0,  2)];
      ring[6] = center[(*this).GetPixelOffset( -2,  2)];
      ring[7] = center[(*this).GetPixelOffset( -2,  0)];
    }

  }

  /**
    * Extractkernel  of the pixel center.
    *
    * @param center        pixel of which the ring is extracted .
    * @param kernel.       kernel of pixel that is extracted from center.
    * @param width,height  shape of the kernel.
    */

  void ExtractKernel(T* center,T* kernel,int width,int height)const {
    CHECK(_image!=NULL, "image not available");
    int x, y;
    for (y = -(height) / 2; y <= (height) / 2; y++)
      for (x = -(width) / 2; x <= (width) / 2; x++)
        *kernel++ = center[this->GetPixelOffset(x, y)];
  }


  /**
   * Gets the (x, y) position of a color of a Bayer image.
   *
   * @param image         Source @ref IMG_image_t "image".
   * @param color         @ref IMG_COLOR "Color plane".
   * @param x             [out] X position, either 0 or 1.
   * @param y             [out] Y position, either 0 or 1.
   *
   * @note For #IMG_COLOR_G and #IMG_COLOR_2G, the first green pixel position is
   *       returned (Y position is always 0).
   * @note This function takes care of @ref IMG_SetBayerJogs "Bayer jogs".
   */
  void GetBayerColorPosition(Image::ChannelType color, int *x, int *y) const
{
  CHECK(_image!=NULL, "image not available");
  CHECK(this->SeemsBayer(), "GetBayerColorPosition() only valid for bayer images");
  // Jog translation:
  //     HV       XY     XY     XY       XY         XY
  // in: 00  gir: 00  r: 10  b: 01  gib: 11  1st g: 00
  //     01       01     11     00       10         10
  //     10       10     00     11       01         10
  //     11       11     01     10       00         00

  switch (color) {
  case Image::GreenOnRed:
    *x = this->GetHorizontalJog() ? 1 : 0;
    *y = this->GetVerticalJog() ? 1 : 0;
    break;
  case Image::Red:
    *x = this->GetHorizontalJog() ? 0 : 1;
    *y = this->GetVerticalJog() ? 1 : 0;
    break;
  case Image::Blue:
    *x = this->GetHorizontalJog() ? 1 : 0;
    *y = this->GetVerticalJog() ? 0 : 1;
    break;
  case Image::GreenOnBlue:
    *x = this->GetHorizontalJog() ? 0 : 1;
    *y = this->GetVerticalJog() ? 0 : 1;
    break;
  default:
    assert(false);
  }
}


  void ReplacePlane(Image::ChannelType color, ImageT<T>& plane)
  {
    CHECK(_image!=NULL, "image not available");
    int i;
    T *data, *cdata;

    assert(this->IsValidImage());
    assert(plane.IsValidImage());

    if (this->GetDataType() == Image::RGB || this->GetDataType() == Image::YCbCr) {
      int size;

      assert(color >= Image::Red && color <= Image::Blue);

      CHECK(plane.GetWidth() == this->GetWidth() && plane.GetHeight() == this->GetHeight(),
        "Plane must be the same size as the image");

      size = this->GetWidth()*this->GetHeight();
      data = this->GetData() + color;
      cdata = plane.GetData();

      for (i = 0; i < size; i++, data += 3)
        *data = *cdata++;
    }
    else {
      int j, width, height, x, y;

      CHECK(plane.GetWidth() == this->GetWidth() / 2
          && plane.GetHeight() == this->GetHeight() / 2,
        "Plane must be 1/4 of the image");

      this->GetBayerColorPosition( color, &x, &y);

      width = this->GetWidth();
      height = this->GetHeight();
      data = this->GetPixelPtr(x, y);
      cdata = plane.GetData();

      if (color == Image::Green) { // Average greens.
        T *data2 = this->GetPixelPtr( x == 0 ? 1 : 0, 1);

        for (i = 0; i < height; i += 2, data += width, data2 += width)
          for (j = 0; j < width; j += 2, data += 2, data2 += 2)
            *data = *data2 = *cdata++;
      }
      else {
        for (i = 0; i < height; i += 2, data += width)
          for (j = 0; j < width; j += 2, data += 2)
            *data = *cdata++;
      }
    }
  }

};

////////////////////////////////////////////////////////////////////////////////
// Dispatch macros


/**
 * Dispatches a function call with one image of all buffer types.
 *
 * @param fn            Function name.
 * @param imageArray    Array of Nathair source images
 * @param nbImages      Size of Nathair source images
 * @param image2        Additional source/target image
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_ARRAY_INPUT_OUTPUT_INT_SAME(fn, inputArray, nbInputImages, outputArray, nbOutputImages, ...) \
    do { \
      int i;\
      if ((nbInputImages>0 && inputArray[0]->GetBufferType() == Image::int16) || \
          (nbOutputImages>0 && outputArray[0]->GetBufferType() == Image::int16)) { \
        ImageT<short> ImageTInputArrayImageTArray[nbInputImages];       \
        ImageT<short> ImageTOutputArrayImageTArray[nbOutputImages];     \
        for(i=0;i<nbInputImages;i++) {ImageTInputArrayImageTArray[i].SetImage(inputArray[i]); } \
        for(i=0;i<nbOutputImages;i++) {ImageTOutputArrayImageTArray[i].SetImage(outputArray[i]); } \
        fn(ImageTInputArrayImageTArray, nbInputImages, ImageTOutputArrayImageTArray, nbOutputImages, ##__VA_ARGS__); \
      } \
      else if ((nbInputImages>0 && inputArray[0]->GetBufferType() == Image::int32) || \
               (nbOutputImages>0 && outputArray[0]->GetBufferType() == Image::int32)) { \
        ImageT<int> ImageTInputArrayImageTArray[nbInputImages];         \
        ImageT<int> ImageTOutputArrayImageTArray[nbOutputImages];       \
        for(i=0;i<nbInputImages;i++) {ImageTInputArrayImageTArray[i].SetImage(inputArray[i]); } \
        for(i=0;i<nbOutputImages;i++) {ImageTOutputArrayImageTArray[i].SetImage(outputArray[i]); } \
        fn(ImageTInputArrayImageTArray, nbInputImages, ImageTOutputArrayImageTArray, nbOutputImages, ##__VA_ARGS__); \
      }  \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

#define DISPATCH_IMAGE_ARRAY_INPUT_OUTPUT_ALL_SAME(fn, inputArray, nbInputImages, outputArray, nbOutputImages, ...) \
    do { \
      int i;\
      if ((nbInputImages>0 && inputArray[0]->GetBufferType() == Image::int16) || \
          (nbOutputImages>0 && outputArray[0]->GetBufferType() == Image::int16)) { \
        ImageT<short> ImageTInputArrayImageTArray[nbInputImages];       \
        ImageT<short> ImageTOutputArrayImageTArray[nbOutputImages];     \
        for(i=0;i<nbInputImages;i++) {ImageTInputArrayImageTArray[i].SetImage(inputArray[i]); } \
        for(i=0;i<nbOutputImages;i++) {ImageTOutputArrayImageTArray[i].SetImage(outputArray[i]); } \
        fn(ImageTInputArrayImageTArray, nbInputImages, ImageTOutputArrayImageTArray, nbOutputImages, ##__VA_ARGS__); \
      } \
      else if ((nbInputImages>0 && inputArray[0]->GetBufferType() == Image::int32) || \
               (nbOutputImages>0 && outputArray[0]->GetBufferType() == Image::int32)) { \
        ImageT<int> ImageTInputArrayImageTArray[nbInputImages];         \
        ImageT<int> ImageTOutputArrayImageTArray[nbOutputImages];       \
        for(i=0;i<nbInputImages;i++) {ImageTInputArrayImageTArray[i].SetImage(inputArray[i]); } \
        for(i=0;i<nbOutputImages;i++) {ImageTOutputArrayImageTArray[i].SetImage(outputArray[i]); } \
        fn(ImageTInputArrayImageTArray, nbInputImages, ImageTOutputArrayImageTArray, nbOutputImages, ##__VA_ARGS__); \
      }                                                                 \
      else if ((nbInputImages>0 && inputArray[0]->GetBufferType() == Image::float32) || \
               (nbOutputImages>0 && outputArray[0]->GetBufferType() == Image::float32)) { \
        ImageT<float> ImageTInputArrayImageTArray[nbInputImages];         \
        ImageT<float> ImageTOutputArrayImageTArray[nbOutputImages];       \
        for(i=0;i<nbInputImages;i++) {ImageTInputArrayImageTArray[i].SetImage(inputArray[i]); } \
        for(i=0;i<nbOutputImages;i++) {ImageTOutputArrayImageTArray[i].SetImage(outputArray[i]); } \
        fn(ImageTInputArrayImageTArray, nbInputImages, ImageTOutputArrayImageTArray, nbOutputImages, ##__VA_ARGS__); \
      }                                                                 \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)


/**
 * Dispatches a function call with one image of all buffer types.
 *
 * @param fn            Function name.
 * @param imageArray    Array of Nathair source images
 * @param nbImages      Size of Nathair source images
 * @param image2        Additional source/target image
 * @param ...           Additional arguments.
 */

#define DISPATCH_IMAGE_ARRAY_INPUT_INT_SAME(fn, imageArray, nbImages, image2, ...) \
    do { \
      int x;\
      if (image2->GetBufferType() == Image::int16) { \
        ImageT<short>* ImageTArray[nbImages];\
        ImageT<short> imageT2(image2); \
        for(x=0;x<nbImages;x++) {ImageTArray[x]= new ImageT<short>(imageArray[x]); }\
        fn(ImageTArray, imageT2 , ##__VA_ARGS__); \
      } \
      else if (image2->GetBufferType() == Image::int32) { \
        ImageT<int> imageT2(image2); \
        ImageT<int>* ImageTArray[nbImages];\
        for(x=0;x<nbImages;x++){ ImageTArray[x]= new ImageT<int>(imageArray[x]); }\
        fn(ImageTArray, imageT2 , ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)


#define DISPATCH_IMAGE_ARRAY_INT(fn, outputImage, nbImages, imageArray, ...) \
    do { \
      int x;\
      if (nbImages>0) {\
        if (imageArray[0]->GetBufferType() == Image::int16) { \
        ImageT<short>* imageTArray[nbImages];\
        for(x=0;x<nbImages;x++) {imageTArray[x]= new ImageT<short>(imageArray[x]); }\
        DISPATCH_IMAGE_INT(fn, outputImage, imageTArray, ##__VA_ARGS__); \
      } \
       else if (imageArray[0]->GetBufferType() == Image::int32) {   \
        ImageT<short>* imageTArray[nbImages];\
        for(x=0;x<nbImages;x++) {imageTArray[x]= new ImageT<short>(imageArray[x]); }\
        DISPATCH_IMAGE_INT(fn, outputImage, imageTArray, ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "New Non supported image buffer type"); \
        } \
    } while (false)

/**
 * Dispatches a function call with one image of all buffer types.
 *
 * @param fn            Function name.
 * @param image         Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */

#define DISPATCH_IMAGE_ALL(fn, image, ...) \
    do { \
      if (image->GetBufferType() == Image::int16) { \
        ImageT<short> imageT(image); \
        fn(imageT , ##__VA_ARGS__); \
      } \
      else if (image->GetBufferType() == Image::int32) { \
        ImageT<int> imageT(image); \
        fn(imageT , ##__VA_ARGS__); \
      } \
      else if (image->GetBufferType() == Image::float32) { \
        ImageT<float> imageT(image); \
        fn(imageT , ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

/**
 * Dispatches a function call with one image of integer buffer types.
 *
 * @param fn            Function name.
 * @param image         Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_INT(fn, image, ...) \
    do { \
      if (image->GetBufferType() == Image::int16) { \
        ImageT<short> imageT(image); \
        fn(imageT , ##__VA_ARGS__); \
      } \
      else if (image->GetBufferType() == Image::int32) { \
        ImageT<int> imageT(image); \
        fn(imageT , ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

/**
 * Dispatches a function call with two images of all buffer types.
 *
 * @param fn            Function name.
 * @param image1        Pointer to the Nathair image.
 * @param image2        Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_ALL_ALL(fn, image1, image2, ...) \
    do { \
      if (image2->GetBufferType() == Image::int16) { \
        ImageT<short> imageT2(image2); \
        DISPATCH_IMAGE_ALL(fn, image1, imageT2 , ##__VA_ARGS__); \
      } \
      else if (image2->GetBufferType() == Image::int32) { \
        ImageT<int> imageT2(image2); \
        DISPATCH_IMAGE_ALL(fn, image1, imageT2 , ##__VA_ARGS__); \
      } \
      else if (image2->GetBufferType() == Image::float32) { \
        ImageT<float> imageT2(image2); \
        DISPATCH_IMAGE_ALL(fn, image1, imageT2 , ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

/**
 * Dispatches a function call with two images of integer buffer types.
 *
 * @param fn            Function name.
 * @param image1        Pointer to the Nathair image.
 * @param image2        Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_INT_INT(fn, image1, image2, ...) \
    do { \
      if (image2->GetBufferType() == Image::int16) { \
        ImageT<short> imageT2(image2); \
        DISPATCH_IMAGE_INT(fn, image1, imageT2 , ##__VA_ARGS__); \
      } \
      else if (image2->GetBufferType() == Image::int32) { \
        ImageT<int> imageT2(image2); \
        DISPATCH_IMAGE_INT(fn, image1, imageT2 , ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

/**
 * Dispatches a function call with three images of integer buffer types.
 *
 * @param fn            Function name.
 * @param image1        Pointer to the Nathair image.
 * @param image2        Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_INT_INT_INT(fn, image1, image2, image3, ...)      \
    do { \
      if (image3->GetBufferType() == Image::int16) { \
        ImageT<short> imageT3(image3); \
        DISPATCH_IMAGE_INT_INT(fn, image1, image2, imageT3 , ##__VA_ARGS__); \
      } \
      else if (image3->GetBufferType() == Image::int32) { \
        ImageT<int> imageT3(image3); \
        DISPATCH_IMAGE_INT_INT(fn, image1, image2, imageT3 , ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

/**
 * Dispatches a function call with four images of integer buffer types.
 *
 * @param fn            Function name.
 * @param image1        Pointer to the Nathair image.
 * @param image2        Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_INT_INT_INT_INT(fn, image1, image2, image3, image4, ...) \
    do { \
      if (image4->GetBufferType() == Image::int16) { \
        ImageT<short> imageT4(image4); \
        DISPATCH_IMAGE_INT_INT_INT(fn, image1, image2, image3, imageT4 , ##__VA_ARGS__); \
      } \
      else if (image4->GetBufferType() == Image::int32) { \
        ImageT<int> imageT4(image4); \
        DISPATCH_IMAGE_INT_INT_INT(fn, image1, image2, image3, imageT4 , ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

/**
 * Dispatches a function call with two images, first image of all buffer types,
 * second image of the same type as the first one.
 *
 * @param fn            Function name.
 * @param image1        Pointer to the Nathair image.
 * @param image2        Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_ALL_SAME(fn, image1, image2, ...) \
    do { \
      if (image1->GetBufferType() == Image::int16) { \
        ImageT<short> imageT1(image1); \
        ImageT<short> imageT2(image2); \
        fn(imageT1, imageT2 , ##__VA_ARGS__); \
      } \
      else if (image1->GetBufferType() == Image::int32) { \
        ImageT<int> imageT1(image1); \
        ImageT<int> imageT2(image2); \
        fn(imageT1, imageT2 , ##__VA_ARGS__); \
      } \
      else if (image1->GetBufferType() == Image::float32) { \
        ImageT<float> imageT1(image1); \
        ImageT<float> imageT2(image2); \
        fn(imageT1, imageT2 , ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

/**
 * Dispatches a function call with two images, first image of integer buffer types,
 * second image of the same type as the first one.
 *
 * @param fn            Function name.
 * @param image1        Pointer to the Nathair image.
 * @param image2        Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_INT_SAME(fn, image1, image2, ...) \
    do { \
      if (image1->GetBufferType() == Image::int16) { \
        ImageT<short> imageT1(image1); \
        ImageT<short> imageT2(image2); \
        fn(imageT1, imageT2 , ##__VA_ARGS__); \
      } \
      else if (image1->GetBufferType() == Image::int32) { \
        ImageT<int> imageT1(image1); \
        ImageT<int> imageT2(image2); \
        fn(imageT1, imageT2 , ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

 /**
 * Dispatches a function call with four images, first image of integer buffer types,
 * second image of the same type as the first one and third idem.
 *
 * @param fn            Function name.
 * @param image1        Pointer to the Nathair image.
 * @param image2        Pointer to the Nathair image.
 * @param image3        Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_INT_SAME_THREE(fn, image1, image2, image3, ...) \
    do { \
      if (image1->GetBufferType() == Image::int16) { \
        ImageT<short> imageT1(image1); \
        ImageT<short> imageT2(image2); \
        ImageT<short> imageT3(image3); \
        fn(imageT1, imageT2 , imageT3, ##__VA_ARGS__); \
      } \
      else if (image1->GetBufferType() == Image::int32) { \
        ImageT<int> imageT1(image1); \
        ImageT<int> imageT2(image2); \
        ImageT<int> imageT3(image3); \
        fn(imageT1, imageT2 , imageT3, ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)

 /**
 * Dispatches a function call with four images, first image of integer buffer types,
 * second image of the same type as the first one (idem for 3rd and 4th).
 *
 * @param fn            Function name.
 * @param image1        Pointer to the Nathair image.
 * @param image2        Pointer to the Nathair image.
 * @param image3        Pointer to the Nathair image.
 * @param image4        Pointer to the Nathair image.
 * @param ...           Additional arguments.
 */
#define DISPATCH_IMAGE_INT_SAME_FOUR(fn, image1, image2, image3, image4, ...) \
    do { \
      if (image1->GetBufferType() == Image::int16) { \
        ImageT<short> imageT1(image1); \
        ImageT<short> imageT2(image2); \
        ImageT<short> imageT3(image3); \
        ImageT<short> imageT4(image4); \
        fn(imageT1, imageT2 , imageT3 , imageT4, ##__VA_ARGS__); \
      } \
      else if (image1->GetBufferType() == Image::int32) { \
        ImageT<int> imageT1(image1); \
        ImageT<int> imageT2(image2); \
        ImageT<int> imageT3(image3); \
        ImageT<int> imageT4(image4); \
        fn(imageT1, imageT2 , imageT3 , imageT4, ##__VA_ARGS__); \
      } \
      else \
        CHECK(false, "Non supported image buffer type"); \
    } while (false)


/** @} */

// backward compatibility to be removed
#define extractKernel ExtractKernel
#define extractBayerRing ExtractBayerRing
#define seemsBayer SeemsBayer
#define copyData CopyData
#define copyBorder CopyBorder
#define getPixelOffset GetPixelOffset
#define getPixelPtr GetPixelPtr

#endif // NATHAIR

#endif // __INCLUDE_RAID_H__
