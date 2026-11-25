/* ss3ds: one version string for the log header, the settings app and the HOME Menu title.
   AUTOMATIC: soulsilver3ds/full/Makefile writes build/ss3ds_buildid.h at every build from git (commit count = run number,
   short hash, "+" if the tree has uncommitted edits). Never edit this by hand. The fallback below only shows when git is unavailable. */
#ifndef SS3DS_VERSION_H
#define SS3DS_VERSION_H
#if defined(__has_include)
#if __has_include("ss3ds_buildid.h")
#include "ss3ds_buildid.h"
#endif
#endif
#ifndef SS3DS_VERSION
#define SS3DS_VERSION "run 0 (no git)"
#endif
#endif
