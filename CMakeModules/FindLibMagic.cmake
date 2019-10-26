
#-------------------------------------------------------------------------------
# Copyright (c) 2013-2013, Lars Baehren <lbaehren@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#  * Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#-------------------------------------------------------------------------------

# - Check for the presence of LIBMAGIC
#
# The following variables are set when LIBMAGIC is found:
#  LIBMAGIC_FOUND      = Set to true, if all components of LIBMAGIC have been
#                        found.
#  LIBMAGIC_INCLUDES   = Include path for the header files of LIBMAGIC
#  LIBMAGIC_LIBRARIES  = Link these to use LIBMAGIC
#  LIBMAGIC_LFLAGS     = Linker flags (optional)

if (NOT LIBMAGIC_FOUND)

  if (NOT LIBMAGIC_ROOT_DIR)
    set (LIBMAGIC_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
  endif (NOT LIBMAGIC_ROOT_DIR)

  ##____________________________________________________________________________
  ## Check for the header files

  find_path (LIBMAGIC_FILE_H
    NAMES file/file.h
    HINTS ${LIBMAGIC_ROOT_DIR} ${CMAKE_INSTALL_PREFIX}
    PATH_SUFFIXES include
    )
  if (LIBMAGIC_FILE_H)
    list (APPEND LIBMAGIC_INCLUDES ${LIBMAGIC_FILE_H})
  endif (LIBMAGIC_FILE_H)

  find_path (LIBMAGIC_MAGIC_H
    NAMES magic.h
    HINTS ${LIBMAGIC_ROOT_DIR} ${CMAKE_INSTALL_PREFIX}
    PATH_SUFFIXES include include/linux
    )
  if (LIBMAGIC_MAGIC_H)
    list (APPEND LIBMAGIC_INCLUDES ${LIBMAGIC_MAGIC_H})
  endif (LIBMAGIC_MAGIC_H)

  list (REMOVE_DUPLICATES LIBMAGIC_INCLUDES)

  ##____________________________________________________________________________
  ## Check for the library

  find_library (LIBMAGIC_LIBRARIES magic
    HINTS ${LIBMAGIC_ROOT_DIR} ${CMAKE_INSTALL_PREFIX}
    PATH_SUFFIXES lib
    )

  ##____________________________________________________________________________
  ## Actions taken when all components have been found

  #find_package_handle_standard_args (LIBMAGIC DEFAULT_MSG LIBMAGIC_LIBRARIES LIBMAGIC_INCLUDES)

  if (LIBMAGIC_FOUND)
    if (NOT LIBMAGIC_FIND_QUIETLY)
      message (STATUS "Found components for LIBMAGIC")
      message (STATUS "LIBMAGIC_ROOT_DIR  = ${LIBMAGIC_ROOT_DIR}")
      message (STATUS "LIBMAGIC_INCLUDES  = ${LIBMAGIC_INCLUDES}")
      message (STATUS "LIBMAGIC_LIBRARIES = ${LIBMAGIC_LIBRARIES}")
    endif (NOT LIBMAGIC_FIND_QUIETLY)
  else (LIBMAGIC_FOUND)
    if (LIBMAGIC_FIND_REQUIRED)
      message (FATAL_ERROR "Could not find LIBMAGIC!")
    endif (LIBMAGIC_FIND_REQUIRED)
  endif (LIBMAGIC_FOUND)

  ##____________________________________________________________________________
  ## Mark advanced variables

  mark_as_advanced (
    LIBMAGIC_ROOT_DIR
    LIBMAGIC_INCLUDES
    LIBMAGIC_LIBRARIES
    )

endif (NOT LIBMAGIC_FOUND)
