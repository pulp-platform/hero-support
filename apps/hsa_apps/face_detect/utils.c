/*******************************************************************************
 *                            C STMicroelectronics
 *    Reproduction and Communication of this document is strictly prohibited 
 *      unless specifically authorized in writing by STMicroelectronics.
 *------------------------------------------------------------------------------
 *                   		Imaging Division 
 *------------------------------------------------------------------------------
 * $Id: utils.c.rca 1.25 Tue Mar 15 15:54:05 2011 stephane.drouard@st.com Experimental $
 *------------------------------------------------------------------------------
 * $Log: utils.c.rca $
 * 
 *  Revision: 1.25 Tue Mar 15 15:54:05 2011 stephane.drouard@st.com
 *  Added GetArrayBuffer().
 * 
 *  Revision: 1.24 Thu Sep 23 14:48:10 2010 stephane.drouard@st.com
 *  Added a function to translate a CPP exception to a Python exception to replace of the one delivered by Cython which does not acquire the GIL.
 * 
 *  Revision: 1.23 Tue Mar 23 12:34:02 2010 brian.stewart@st.com
 *  removed exit call when building dll/so, that is when SHARED is define
 *  don't want to exit from shard library this is going to bring the calling program crashing down
 * 
 *  Revision: 1.22 Tue Mar 31 18:52:38 2009 stephane.drouard@st.com
 *  Fixed missing inclusion of stdexcept.
 * 
 *  Revision: 1.21 Fri Dec 19 14:32:34 2008 stephane.drouard@st.com
 *  Now calls SetModuleName().
 * 
 *  Revision: 1.20 Thu Nov 20 10:06:43 2008 stephane.drouard@st.com
 *  Added GetModuleName() for NATHAIR.
 * 
 *  Revision: 1.19 Wed Oct 29 17:42:44 2008 stephane.drouard@st.com
 *  Added check of the C++ type to FromSwig().
 * 
 *  Revision: 1.18 Wed Oct 29 14:24:31 2008 stephane.drouard@st.com
 *  Nathair: added FromSwig() and AsImage().
 * 
 *  Revision: 1.17 Thu Oct 16 10:48:45 2008 stephane.drouard@st.com
 *  Replaced VERBOSE() by WARNING()/INFO()/DEBUG() (need to delete trailing "\n").
 *  GetAppName() now only available in standalone.
 *  SetAppName() renamed in SetModuleName() (Nathair only).
 *  error.c removed and merged with utils.c (need to clean before 1st rebuild).
 *  Reworked on the documentation.
 * 
 *  Revision: 1.16 Wed Mar  5 17:35:53 2008 stephane.drouard@st.com
 *  Fixed some Nathair functions not documented.
 * 
 *  Revision: 1.15 Thu Feb  7 17:35:52 2008 stephane.drouard@st.com
 *  Added Nathair support.
 * 
 *  Revision: 1.14 Tue Jan 15 15:05:55 2008 stephane.drouard@st.com
 *  Added check of either STANDALONE or SOFTIP defined at compile time.
 * 
 *  Revision: 1.13 Wed Apr 18 10:17:29 2007 stephane.drouard@st.com
 *  Added module parameter to TRY_RAID_LIBRARY().
 * 
 *  Revision: 1.12 Fri Apr 13 17:22:34 2007 stephane.drouard@st.com
 *  Added SoftIP environment support.
 *  Updated comments.
 *  Changed IMG_Create(): now lets the buffer uninitialized (performance, not backward compatible).
 *  Changed IMG_Copy(): now supports overlapping regions.
 * 
 *  Revision: 1.11 Wed Apr  4 16:31:06 2007 stephane.drouard@st.com
 *  Adapted to new RAID structure.
 * 
 *  Revision: 1.10 Tue Feb 20 10:28:14 2007 stephane.drouard@st.com
 *  Changed CMDLINE syntax.
 * 
 *  Revision: 1.9 Wed Jan 31 18:56:40 2007 stephane.drouard@st.com
 *  Modified VERBOSE macro: "\n" is no more automatically appended.
 * 
 *  Revision: 1.8 Mon Dec  4 17:05:30 2006 stephane.drouard@st.com
 *  Fixed documentation of strcasestr().
 * 
 *  Revision: 1.7 Tue Nov 28 18:43:07 2006 stephane.drouard@st.com
 *  Updated comments.
 *  Updated error messages.
 * 
 *  Revision: 1.6 Tue Nov 28 15:35:30 2006 stephane.drouard@st.com
 *  Added VERBOSE() macro.
 * 
 *  Revision: 1.5 Fri Nov 17 15:05:52 2006 stephane.drouard@st.com
 *  Added some string functions:
 *    strcasechr(),
 *    strcaserchr(),
 *    strcasestr(),
 *    TrimLeft(),
 *    TrimRight().
 * 
 *  Revision: 1.4 Wed Oct 25 18:08:36 2006 stephane.drouard@st.com
 *  Removed bool_t, FALSE and TRUE, replaced by the C99's standard one (bool, false and true).
 *  Added CHECK() and SYSTEM_CHECK() macros.
 *  Rewrote CMDLINE macros.
 *  Added IMG_MAX_WIDTH/HEIGHT.
 * 
 *  Revision: 1.3 Wed Aug 23 15:23:35 2006 stephane.drouard@st.com
 *  Updated doc
 * 
 *  Revision: 1.2 Tue Aug 22 11:21:49 2006 olivier.lemarchand@st.com
 *  Added Synchronicity header.
 ******************************************************************************/
#include "raid.h"
// 
// #include <limits.h>
// #include <ctype.h>
// #include <stdarg.h>
// #include <errno.h>
// 
// /**
//  * @addtogroup UTILS
//  * @{
//  */
// 
// #if defined(STANDALONE) || defined(DOXYGEN)
// 
// /**
//  * @internal
//  *
//  * Application name.
//  */
// static const char *__appName;
// 
// /**
//  * @internal
//  *
//  * Sets the application name from argv[0].
//  *
//  * This function is used by CMDLINE_START() and should not be directly called.
//  *
//  * @param argv0         The application path.
//  */
// void __SetAppName(const char *argv0)
// {
//   const char *p = strrchr(argv0, '/');
//   if (p == NULL)
//     p = strrchr(argv0, '\\');
//   __appName = p == NULL ? argv0 : p + 1;
// }
// 
// /**
//  * Gets the application name.
//  *
//  * @par Specific to the Standalone environment.
//  *
//  * @return              The application name.
//  *
//  * @note Can only be used once CMDLINE_START() has been called.
//  */
// const char *GetAppName(void)
// {
//   assert(__appName != NULL);
//   return __appName;
// }
// 
// #endif // defined(STANDALONE)
// 
// #if defined(SOFTIP) || defined(DOXYGEN)
// 
// /**
//  * @internal
//  *
//  * Module name.
//  */
// static const char *__module;
// 
// /**
//  * @internal
//  *
//  * setjmp()/longjmp() buffer for error handling.
//  */
// static jmp_buf __jmpBuffer;
// 
// /**
//  * @internal
//  *
//  * Installs the simple try/catch mechanism.
//  *
//  * @param module        Module name.
//  * @return              A jmp_buf storage.
//  */
// void *__TryRaidLib(const char *module)
// {
//   assert(__module == NULL);
//   __module = module;
//   return __jmpBuffer;
// }
// 
// /**
//  * @internal
//  *
//  * Uninstalls the simple try/catch mechanism.
//  */
// void __EndTryRaidLib(void)
// {
//   assert(__module != NULL);
//   __module = NULL;
// }
// 
// #endif // defined(SOFTIP)
// 
// #if defined(NATHAIR) || defined(DOXYGEN)
// 
// #include <stdexcept>
// 
// /**
//  * @internal
//  *
//  * Module name.
//  */
// static const char *__module;
// 
// /**
//  * @internal
//  *
//  * This variable is used by verbose modes, initialized by SetModuleName().
//  */
// Logger __logger;
// 
// /**
//  * Sets the module name.
//  *
//  * @par Specific to the Nathair environment.
//  *
//  * @param name          Name of the module.
//  *
//  * @note Must be called for proper logging mechanism.
//  */
// void SetModuleName(const char *name)
// {
//   assert(name != NULL);
//   assert(__module == NULL); // Already set.
//   __module = name;
//   __logger.pySetLogger(name);
// }
// 
// /**
//  * Gets the module name.
//  *
//  * @par Specific to the Nathair environment.
//  *
//  * @return              Name of the module.
//  */
// const char *GetModuleName(void)
// {
//   assert(__module != NULL);
//   return __module;
// }
// 
// /*
//  * Translates a CPP exception to a Python exception.
//  * This function is used in replacement of the one delivered by Cython in order
//  * to acquire the lock.
//  */
// void __CppExn2PyErr() {
//   PyGILState_STATE state = PyGILState_Ensure();
//   try {
//     if (PyErr_Occurred())
//       ; // let the latest Python exn pass through and ignore the current one
//     else
//       throw;
//   } catch (const std::out_of_range& exn) {
//     // catch out_of_range explicitly so the proper Python exn may be raised
//     PyErr_SetString(PyExc_IndexError, exn.what());
//   } catch (const std::exception& exn) {
//     PyErr_SetString(PyExc_RuntimeError, exn.what());
//   }
//   catch (...) {
//     PyErr_SetString(PyExc_RuntimeError, "Unknown exception");
//   }
//   PyGILState_Release(state);
// }
// 
// /**
//  * Retrieves the data pointer from a Numpy object.
//  *
//  * @par Specific to the Nathair environment.
//  *
//  * @param obj           Numpy object.
//  * @param size          Item size.
//  * @return              Data pointer.
//  *
//  * @warning The Numpy object must not be deleted while the pointer is used.
//  */
// void* GetArrayBuffer(PyArrayObject* obj, int size)
// {
//   CHECK(PyArray_ITEMSIZE(obj) == size, "Item sizes do not match");
//   CHECK(PyArray_CHKFLAGS(obj, NPY_C_CONTIGUOUS | NPY_ALIGNED),
//     "Not a C contiguous buffer");
//   return PyArray_DATA(obj);
// }
// 
// #endif // defined(NATHAIR)
// 
// /**
//  * Case-insensitive version of strchr().
//  */
// /*char *strcasechr(const char *s, int c)
// {
//   c = toupper(c);
//   for (; *s != '\0'; s++)
//     if (toupper(*s) == c)
//       return (char*)s;
// 
//   return NULL;
// }
// */
// /**
//  * Case-insensitive version of strrchr().
//  */
// /*char *strcaserchr(const char *s, int c)
// {
//   char *p = NULL;
// 
//   c = toupper(c);
//   for (; *s != '\0'; s++)
//     if (toupper(*s) == c)
//       p = (char*)s;
// 
//   return p;
// }
// */
// /**
//  * Case-insensitive version of strstr().
//  */
// /*char *strcasestr(const char *s1, const char *s2)
// {
//   size_t len1 = strlen(s1);
//   size_t len2 = strlen(s2);
// 
//   for (; len2 <= len1; s1++, len1--)
//     if (strncasecmp(s1, s2, len2) == 0)
//       return (char*)s1;
// 
//   return NULL;
// }
// */
// /**
//  * Skips left white-space characters.
//  *
//  * @param s             String.
//  * @return              A pointer on the first non-white-space character.
//  *
//  * @note The definition of a white-space character is the same as @p isspace().
//  */
// char *TrimLeft(const char *s)
// {
//   while (isspace((int)*s))
//     s++;
//   return (char*)s;
// }
// 
// /**
//  * Removes right white-space characters.
//  *
//  * @param s             String.
//  * @return              @p s.
//  *
//  * @note The definition of a white-space character is the same as @p isspace().
//  */
// char *TrimRight(char *s)
// {
//   char *s2 = s + strlen(s);
//   while (s2 > s && isspace((int)s2[-1]))
//     s2--;
//   *s2 = 0;
//   return s;
// }

#if defined(STANDALONE) || defined(DOXYGEN)

/**
 * @internal
 *
 * This variable is used by verbose modes and should not be directly accessed.
 */
char __verbose = 0;

/**
 * @internal
 *
 * Displays a verbose message.
 *
 * This function is used by WARNING(), INFO() and DEBUG() macros and should not
 * be directly called.
 *
 * The message is printed on the standard error.
 *
 * @param level         Level.
 * @param msg           Message to be displayed in printf format.
 * @param ...           Message's variable arguments.
 */
void __Verbose(int level, const char *msg, ...)
{
//   static const char *levels[] = { "WARNING", "INFO", "DEBUG" };
//   va_list list;
// 
//   assert(msg != NULL);
//   assert(level >= 0 && level < countof(levels));
// 
//   va_start(list, msg);
//   fprintf(stderr, "[%s:%s] ", GetAppName(), levels[level]);
//   vfprintf(stderr, msg, list);
//   fputc('\n', stderr);
//   va_end(list);
}

#endif // defined(STANDALONE)

/**
 * @internal
 *
 * Aborts the application with an error message.
 *
 * The message is printed on the standard error.
 *
 * This function is used by CHECK() and should not be directly called.
 *
 * @param file          Source file name.
 * @param line          Source line number.
 * @param msg           Error message in printf format.
 * @param ...           Error message's variable arguments.
 */
void __Error(const char *file, int line, const char *msg, ...)
{
//   va_list list;
//   #if defined(SOFTIP)
//   char str[VIMG_MAX_ERROR_STRING];
//   char *errors[1];
//   #elif defined(NATHAIR)
//   char str[1024];
//   #endif
// 
//   assert(msg != NULL);
// 
//   #if defined(SOFTIP)
// 
//   va_start(list, msg);
//   vsnprintf(str, sizeof(str), msg, list);
//   va_end(list);
//   errors[0] = str;
//   ViperUtils_PrintErrorMessage(__module, ViperErr_FirstForModule, errors,
//     false, true);
//   __EndTryRaidLib();
//   longjmp(__jmpBuffer, 1);
// 
//   #elif defined(NATHAIR)
// 
//   va_start(list, msg);
//   vsnprintf(str, sizeof(str), msg, list);
//   va_end(list);
// 
//   #ifndef NDEBUG
//   const char *p = strrchr(file, '\\');
//   if (p == NULL)
//     p = strrchr(file, '/');
//   if (p != NULL)
//     file = p + 1;
//   __logger.Error("%s (%s:%d)", str, file, line);
//   #endif
// 
//   throw std::runtime_error(str);
// 
//   #else
// 
//   fprintf(stderr, "[%s:ERROR] ", GetAppName());
// 
//   va_start(list, msg);
//   vfprintf(stderr, msg, list);
//   va_end(list);
// 
//   #ifndef NDEBUG
//   if (__verbose > 1) {
//     const char *p = strrchr(file, '\\');
//     if (p == NULL)
//       p = strrchr(file, '/');
//     if (p != NULL)
//       file = p + 1;
//     fprintf(stderr, " (%s:%d)", file, line);
//   }
//   #endif
// 
//   fputc('\n', stderr);
//   #ifndef SHARED
//   exit(1);
//   #endif // SHARED
// 
//   #endif
    printf("__Error\n");
    while(1);
}

/**
 * @internal
 *
 * Aborts the application with a message corresponding to the last failed
 * system call.
 *
 * The message is printed on the standard error.
 *
 * This function is used by SYSTEM_CHECK() and should not be directly called.
 *
 * @param file          Source file name.
 * @param line          Source line number.
 * @param msg           Error message in printf format to be displayed in front
 *                      of the system message. May be @p NULL.
 * @param ...           Error message's variable arguments.
 */
void __SystemError(const char *file, int line, const char *msg, ...)
{
//   if (msg != NULL) {
//     va_list list;
//     char str[500];
// 
//     va_start(list, msg);
//     vsnprintf(str, sizeof(str), msg, list);
//     va_end(list);
//     __Error(file, line, "%s: %s", str, strerror(errno));
//   }
// 
//   __Error(file, line, "%s", strerror(errno));
    printf("__Error\n");
    while(1);
}

/** @} */
