/* Minimal TRE config for vendored use — no autoconf needed */
#define TRE_VERSION "0.9.0"
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_WCHAR_H 1
#define HAVE_WCTYPE_H 1
#define HAVE_MBSTATE_T 1
/* Do NOT define TRE_MULTIBYTE or TRE_WCHAR — we use single-byte char only.
 * #ifdef checks presence, not value, so defining as 0 still enables them. */
/* #undef TRE_MULTIBYTE */
/* #undef TRE_WCHAR */
#define TRE_APPROX 0
#define TRE_REGEX_T_FIELD value
#define HAVE_MBRTOWC 1
#define HAVE_MBTOWC 1
#define HAVE_WCSLEN 1
#define HAVE_ISASCII 1
#define NDEBUG 1
