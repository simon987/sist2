/*
 Copyright (c) 2010, Florian Reuter
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions 
 are met:
 
 * Redistributions of source code must retain the above copyright 
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright 
   notice, this list of conditions and the following disclaimer in 
   the documentation and/or other materials provided with the 
   distribution.
 * Neither the name of Florian Reuter nor the names of its contributors 
   may be used to endorse or promote products derived from this 
   software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 OF THE POSSIBILITY OF SUCH DAMAGE.
 
*/
/** @file opc/part.h
 
 */
#include <opc/config.h>

#ifndef OPC_PART_H
#define OPC_PART_H

#ifdef __cplusplus
extern "C" {
#endif    
    /**
     Handle to an OPC part created by \ref opcPartOpen.
     \see opcPartOpen.
     */
    typedef xmlChar* opcPart;

/**
  Represents an invalid (resp. NULL) part.
  In releations OPC_PART_INVALID also represents the root part.
  \hideinitializer
  */
#define OPC_PART_INVALID NULL

    /**
     Find a part in a \ container by \c absolutePath and/or \c type.
     Currently no flags are supported.
     */
    opcPart opcPartFind(opcContainer *container, 
                        const xmlChar *absolutePath, 
                        const xmlChar *type,
                        int flags);

    /**
     Creates a part in a \ container with \c absolutePath and \c type.
     Currently no flags are supported.
     */
    opcPart opcPartCreate(opcContainer *container, 
                          const xmlChar *absolutePath, 
                          const xmlChar *type,
                          int flags);

    /**
      Returns the type of the container.
      The string is interned and must not be freed.
      */
    const xmlChar *opcPartGetType(opcContainer *c, opcPart part);

    /**
      Returns the type of the container. 
      If \c override_only then the return value will be NULL for parts not having an override type.
      The string is interned and must not be freed.
      */
    const xmlChar *opcPartGetTypeEx(opcContainer *c, opcPart part, opc_bool_t override_only);

    /**
     Deleted that part \c absolutePath in the \c container.
     */
    opc_error_t opcPartDelete(opcContainer *container, const xmlChar *absolutePath);

    /**
      Get the first part.
      \code
      for(opcPart part=opcPartGetFirst(c);OPC_PART_INVALID!=part;part=opcPartGetNext(c, part)) {
        printf("%s; \n", part, opcPartGetType(c, part));
      }
      \endcode 
      */
    opcPart opcPartGetFirst(opcContainer *container);

    /**
     Get the next part.
     \see opcPartGetFirst
      */
    opcPart opcPartGetNext(opcContainer *container, opcPart part);

    /**
      Returns the size in bytes of the \c part.
      */
    opc_ofs_t opcPartGetSize(opcContainer *c, opcPart part);

#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_PART_H */
