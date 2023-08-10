/* Define for the ARM32 architecture. */
/* #undef ARM32 */

/* Define for the ARM64 architecture. */
/* #undef ARM64 */

/* Define for the i386 architecture. */
/* #undef I386 */

/* Define for the AMD x86-64 architecture. */
#define X86_64

/* Toggle debugging via environment variable LIBVMI_DEBUG */
#define ENV_DEBUG

/* Enable or disable the address cache (v2p, pid, etc) */
#define ENABLE_ADDRESS_CACHE

/* Enable libvmi.conf */
#define ENABLE_CONFIGFILE

/* Define to enable file support. */
#define ENABLE_FILE

/* Define to FreeBSD support. */
#define ENABLE_FREEBSD

/* Define to enable KVM support. */
#define ENABLE_KVM

/* Define to enable KVM legacy driver support */
/* #undef ENABLE_KVM_LEGACY */

/* Define to Linux support. */
#define ENABLE_LINUX

/* Enable or disable the page cache */
#define ENABLE_PAGE_CACHE

/* Enable API safety checks */
#define ENABLE_SAFETY_CHECKS

/* Define to Windows support. */
#define ENABLE_WINDOWS

/* Define to enable Xen support. */
/* #undef ENABLE_XEN */

/* Define to enable Bareflank support. */
/* #undef ENABLE_BAREFLANK */

/* Define if you have <qemu/libvmi_request.h>. */
/* #undef HAVE_LIBVMI_REQUEST */

/* Define if we have Xenstore support. */
/* #undef HAVE_LIBXENSTORE */

/* Define if you have the <xenstore.h> header file. */
/* #undef HAVE_XENSTORE_H */

/* Define if you have the <xs.h> header file. */
/* #undef HAVE_XS_H */

/* xen headers define hvmmem_access_t */
/* #undef HAVE_HVMMEM_ACCESS_T */

/* xen headers define xenmem_access_t */
/* #undef HAVE_XENMEM_ACCESS_T */

/* Max number of pages held in page cache */
#define MAX_PAGE_CACHE_SIZE 512

/* Debug level */
#define VMI_DEBUG ((VMI_DEBUG_CORE | VMI_DEBUG_DRIVER | VMI_DEBUG_KVM))

/* Name of package */
#define PACKAGE "LibVMI"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "LibVMI"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "LibVMI 0.13.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "LibVMI"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.13.0"

/* Defined when working JSON library was found */
#define ENABLE_JSON_PROFILES

/* Defined when working JSON-C library was found to parse Rekall
   profiles. */
#define REKALL_PROFILES

/* Defined when working JSON-C library was found to parse Volatility
   ISTs. */
#define VOLATILITY_IST

/* Version number of package */
#define VERSION "0.13.0"
