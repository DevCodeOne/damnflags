# This file is NOT licensed under the GPLv3, which is the license for the rest
# of YouCompleteMe.
#
# Here's the license text for this file:
#
# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
import os
import json
import ycm_core
import logging

_logger = logging.getLogger(__name__)


def DirectoryOfThisScript():
  return os.path.dirname( os.path.abspath( __file__ ) )

compilation_database_folder = os.path.join(DirectoryOfThisScript())

def GetCompilationInfoForFile( filename ):
  if os.path.exists( compilation_database_folder ):
    database = ycm_core.CompilationDatabase( compilation_database_folder )
    if not database.DatabaseSuccessfullyLoaded():
        _logger.warn("Failed to load database")
        database = None
  else:
    database = None

  return database.GetCompilationInfoForFile( filename )


def FlagsForFile( filename, **kwargs ):
  relative_to = None
  compiler_flags = None

  compilation_info = GetCompilationInfoForFile( filename )
  compiler_flags = compilation_info.compiler_flags_

  for flag in compiler_flags:
      if flag.startswith("-W"):
          compiler_flags.remove(flag)
  _logger.info("Final flags for %s are %s" % (filename, ' '.join(compiler_flags)))

  return {
    'flags': compiler_flags,
    'do_cache': False
  }
