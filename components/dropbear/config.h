/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <pthread.h>
#include <shell/shell.h>

#define getpid() pthread_self()

// dropbear is executed into a task, so when calling to exit function
// simply delete current task
#define exit(e) \
	vTaskDelete(NULL); \
	for(;;);

/* Using AIX */
/* #undef AIX */

/* Broken getaddrinfo */
/* #undef BROKEN_GETADDRINFO */

/* Use bundled libtom */
#define BUNDLED_LIBTOM /**/

/* lastlog file location */
/* #undef CONF_LASTLOG_FILE */

/* utmpx file location */
/* #undef CONF_UTMPX_FILE */

/* utmp file location */
/* #undef CONF_UTMP_FILE */

/* wtmpx file location */
/* #undef CONF_WTMPX_FILE */

/* wtmp file location */
/* #undef CONF_WTMP_FILE */

/* Disable use of lastlog() */
/* #undef DISABLE_LASTLOG */

/* Use PAM */
#define DISABLE_PAM /**/

/* Disable use of pututline() */
/* #undef DISABLE_PUTUTLINE */

/* Disable use of pututxline() */
/* #undef DISABLE_PUTUTXLINE */

/* Using syslog */
/* #undef DISABLE_SYSLOG */

/* Disable use of utmp */
#define DISABLE_UTMP 1

/* Disable use of utmpx */
/* #undef DISABLE_UTMPX */

/* Disable use of wtmp */
#define DISABLE_WTMP 1

/* Disable use of wtmpx */
#define DISABLE_WTMPX 1

/* Use zlib */
/* #undef DISABLE_ZLIB */

/* Define to 1 if you have the `basename' function. */
// #define HAVE_BASENAME 1

/* Define to 1 if you have the `clearenv' function. */
/* #undef HAVE_CLEARENV */

/* Define if gai_strerror() returns const char * */
//#define HAVE_CONST_GAI_STRERROR_PROTO 1

/* crypt() function */
#define HAVE_CRYPT 1

/* Define to 1 if you have the <crypt.h> header file. */
/* #undef HAVE_CRYPT_H */

/* Define to 1 if you have the `daemon' function. */
// #define HAVE_DAEMON 1

/* Use /dev/ptc & /dev/pts */
/* #undef HAVE_DEV_PTS_AND_PTC */

/* Define to 1 if you have the `dup2' function. */
#define HAVE_DUP2 1

/* Define to 1 if you have the `endutent' function. */
/* #undef HAVE_ENDUTENT */

/* Define to 1 if you have the `endutxent' function. */
// #define HAVE_ENDUTXENT 1

/* Define to 1 if you have the `explicit_bzero' function. */
/* #undef HAVE_EXPLICIT_BZERO */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fork' function. */
// #define HAVE_FORK 1

/* Define to 1 if you have the `freeaddrinfo' function. */
#define HAVE_FREEADDRINFO 1

/* Define to 1 if you have the `gai_strerror' function. */
// #define HAVE_GAI_STRERROR 1

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the `getnameinfo' function. */
#define HAVE_GETNAMEINFO 1

/* Define to 1 if you have the `getpass' function. */
// #define HAVE_GETPASS 1

/* Define to 1 if you have the `getspnam' function. */
/* #undef HAVE_GETSPNAM */

/* Define to 1 if you have the `getusershell' function. */
// #define HAVE_GETUSERSHELL 1

/* Define to 1 if you have the `getutent' function. */
/* #undef HAVE_GETUTENT */

/* Define to 1 if you have the `getutid' function. */
/* #undef HAVE_GETUTID */

/* Define to 1 if you have the `getutline' function. */
/* #undef HAVE_GETUTLINE */

/* Define to 1 if you have the `getutxent' function. */
// #define HAVE_GETUTXENT 1

/* Define to 1 if you have the `getutxid' function. */
// #define HAVE_GETUTXID 1

/* Define to 1 if you have the `getutxline' function. */
// #define HAVE_GETUTXLINE 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <ioctl.h> header file. */
/* #undef HAVE_IOCTL_H */

/* Define to 1 if you have the <lastlog.h> header file. */
/* #undef HAVE_LASTLOG_H */

/* Define to 1 if you have the <libgen.h> header file. */
// #define HAVE_LIBGEN_H 1

/* Define to 1 if you have the `pam' library (-lpam). */
/* #undef HAVE_LIBPAM */

/* Define to 1 if you have the <libutil.h> header file. */
/* #undef HAVE_LIBUTIL_H */

/* Define to 1 if you have the `z' library (-lz). */
// #define HAVE_LIBZ 1

/* Define to 1 if you have the <limits.h> header file. */
// #define HAVE_LIMITS_H 1

/* Have login() function */
// #define HAVE_LOGIN /**/

/* Define to 1 if you have the `logout' function. */
// #define HAVE_LOGOUT 1

/* Define to 1 if you have the `logwtmp' function. */
// #define HAVE_LOGWTMP 1

/* Define to 1 if you have the `mach_absolute_time' function. */
// #define HAVE_MACH_ABSOLUTE_TIME 1

/* Define to 1 if you have the <mach/mach_time.h> header file. */
// #define HAVE_MACH_MACH_TIME_H 1

/* Define to 1 if you have the <memory.h> header file. */
// #define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `memset_s' function. */
// #define HAVE_MEMSET_S 1

/* Define to 1 if you have the <netdb.h> header file. */
// #define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
// #define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <netinet/in_systm.h> header file. */
// #define HAVE_NETINET_IN_SYSTM_H 1

/* Define to 1 if you have the <netinet/tcp.h> header file. */
// #define HAVE_NETINET_TCP_H 1

/* Have openpty() function */
#define HAVE_OPENPTY 1

/* Define to 1 if you have the `pam_fail_delay' function. */
/* #undef HAVE_PAM_FAIL_DELAY */

/* Define to 1 if you have the <pam/pam_appl.h> header file. */
/* #undef HAVE_PAM_PAM_APPL_H */

/* Define to 1 if you have the <paths.h> header file. */
// #define HAVE_PATHS_H 1

/* Define to 1 if you have the <pty.h> header file. */
/* #undef HAVE_PTY_H */

/* Define to 1 if you have the `putenv' function. */
// #define HAVE_PUTENV 1

/* Define to 1 if you have the `pututline' function. */
/* #undef HAVE_PUTUTLINE */

/* Define to 1 if you have the `pututxline' function. */
// #define HAVE_PUTUTXLINE 1

/* Define to 1 if you have the <security/pam_appl.h> header file. */
// #define HAVE_SECURITY_PAM_APPL_H 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `setutent' function. */
/* #undef HAVE_SETUTENT */

/* Define to 1 if you have the `setutxent' function. */
// #define HAVE_SETUTXENT 1

/* Define to 1 if you have the <shadow.h> header file. */
/* #undef HAVE_SHADOW_H */

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strlcat' function. */
#define HAVE_STRLCAT 1

/* Define to 1 if you have the `strlcpy' function. */
#define HAVE_STRLCPY 1

/* Define to 1 if you have the <stropts.h> header file. */
/* #undef HAVE_STROPTS_H */

/* Have struct addrinfo */
#define HAVE_STRUCT_ADDRINFO /**/

/* Have struct in6_addr */
#define HAVE_STRUCT_IN6_ADDR /**/

/* Have struct sockaddr_in6 */
#define HAVE_STRUCT_SOCKADDR_IN6 /**/

/* Define to 1 if the system has the type `struct sockaddr_storage'. */
#define HAVE_STRUCT_SOCKADDR_STORAGE 1

/* Define to 1 if `ss_family' is a member of `struct sockaddr_storage'. */
#define HAVE_STRUCT_SOCKADDR_STORAGE_SS_FAMILY 1

/* Define to 1 if `ut_addr' is a member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_ADDR */

/* Define to 1 if `ut_addr_v6' is a member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_ADDR_V6 */

/* Define to 1 if `ut_host' is a member of `struct utmpx'. */
// #define HAVE_STRUCT_UTMPX_UT_HOST 1

/* Define to 1 if `ut_id' is a member of `struct utmpx'. */
// #define HAVE_STRUCT_UTMPX_UT_ID 1

/* Define to 1 if `ut_syslen' is a member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_SYSLEN */

/* Define to 1 if `ut_time' is a member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_TIME */

/* Define to 1 if `ut_tv' is a member of `struct utmpx'. */
// #define HAVE_STRUCT_UTMPX_UT_TV 1

/* Define to 1 if `ut_type' is a member of `struct utmpx'. */
// #define HAVE_STRUCT_UTMPX_UT_TYPE 1

/* Define to 1 if `ut_addr' is a member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_ADDR */

/* Define to 1 if `ut_addr_v6' is a member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_ADDR_V6 */

/* Define to 1 if `ut_exit' is a member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_EXIT */

/* Define to 1 if `ut_host' is a member of `struct utmp'. */
// #define HAVE_STRUCT_UTMP_UT_HOST 1

/* Define to 1 if `ut_id' is a member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_ID */

/* Define to 1 if `ut_pid' is a member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_PID */

/* Define to 1 if `ut_time' is a member of `struct utmp'. */
// #define HAVE_STRUCT_UTMP_UT_TIME 1

/* Define to 1 if `ut_tv' is a member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_TV */

/* Define to 1 if `ut_type' is a member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_TYPE */

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
//#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
// #define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if the system has the type `uint16_t'. */
#define HAVE_UINT16_T 1

/* Define to 1 if the system has the type `uint32_t'. */
#define HAVE_UINT32_T 1

/* Define to 1 if the system has the type `uint8_t'. */
#define HAVE_UINT8_T 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `updwtmp' function. */
/* #undef HAVE_UPDWTMP */

/* Define to 1 if you have the <util.h> header file. */
// #define HAVE_UTIL_H 1

/* Define to 1 if you have the `utmpname' function. */
/* #undef HAVE_UTMPNAME */

/* Define to 1 if you have the `utmpxname' function. */
// #define HAVE_UTMPXNAME 1

/* Define to 1 if you have the <utmpx.h> header file. */
// #define HAVE_UTMPX_H 1

/* Define to 1 if you have the <utmp.h> header file. */
// #define HAVE_UTMP_H 1

/* Define to 1 if the system has the type `u_int16_t'. */
#define HAVE_U_INT16_T 1

/* Define to 1 if the system has the type `u_int32_t'. */
#define HAVE_U_INT32_T 1

/* Define to 1 if the system has the type `u_int8_t'. */
#define HAVE_U_INT8_T 1

/* Define to 1 if you have the `writev' function. */
#define HAVE_WRITEV 1

/* Define to 1 if you have the `_getpty' function. */
/* #undef HAVE__GETPTY */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define to the type of arg 1 for `select'. */
#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for `select'. */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg 5 for `select'. */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Use /dev/ptmx */
//#define USE_DEV_PTMX /**/

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* type to use in place of socklen_t if not defined */
/* #undef socklen_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

#define DROPBEAR_SERVER
#define DROPBEAR_CLIENT
//#define DEBUG_TRACE
//#define LOG_COMMANDS

#define _PATH_DEVNULL "/dev/null"
