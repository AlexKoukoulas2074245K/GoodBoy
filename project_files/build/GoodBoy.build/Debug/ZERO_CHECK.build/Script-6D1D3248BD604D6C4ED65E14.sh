#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/Code/GoodBoy/project_files
  make -f /Users/Code/GoodBoy/project_files/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/Code/GoodBoy/project_files
  make -f /Users/Code/GoodBoy/project_files/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/Code/GoodBoy/project_files
  make -f /Users/Code/GoodBoy/project_files/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/Code/GoodBoy/project_files
  make -f /Users/Code/GoodBoy/project_files/CMakeScripts/ReRunCMake.make
fi

