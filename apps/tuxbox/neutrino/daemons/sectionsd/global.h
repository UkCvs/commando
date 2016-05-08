#ifndef GLOBAL_H_
#define GLOBAL_H_

/* ===========================
 *     sectionsd   macros
 * =========================== */
// prevent use of unsafe or not thread-safe functions - u can add more
#define localtime(a) UNSAFE_LOCALTIME_NOT_THREADSAFE_USE_LOCALTIME_R()
#define ctime(a) UNSAFE_CTIME_NOT_THREADSAFE_USE_CS_CTIME_R()
#define gmtime(a) UNSAFE_GMTIME_NOT_THREADSAFE_USE_CS_GMTIME_R()
#define asctime(a) UNSAFE_ASCTIME_NOT_THREADSAFE_USE_ASCTIME_R()

//#define strcpy(a,b) UNSAFE_STRCPY_USE_CS_STRNCPY_INSTEAD()
//#define sprintf(a,...) UNSAFE_SPRINTF_USE_SNPRINTF_INSTEAD()
//#define strtok(a,b) UNSAFE_STRTOK_USE_STRTOK_R_INSTEAD()

#endif
