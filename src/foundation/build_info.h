/*
 * build_info.h — Build provenance embedded in every executable contract.
 *
 * The Makefile supplies these values for production builds. Defaults make
 * ad-hoc unit builds explicit rather than silently reporting the old `dev`.
 */
#ifndef CBM_BUILD_INFO_H
#define CBM_BUILD_INFO_H

#ifndef CBM_VERSION
#define CBM_VERSION "0.8.1-dev"
#endif

#ifndef CBM_GIT_COMMIT
#define CBM_GIT_COMMIT "unknown"
#endif

#ifndef CBM_GIT_DIRTY
#define CBM_GIT_DIRTY "unknown"
#endif

#ifndef CBM_BUILD_ID
#define CBM_BUILD_ID "source"
#endif

#endif /* CBM_BUILD_INFO_H */
