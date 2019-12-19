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
/** @file opc/relation.h
 
 */
#include <opc/config.h>

#ifndef OPC_RELATION_H
#define OPC_RELATION_H

#ifdef __cplusplus
extern "C" {
#endif    

    /**
     Indentifier for an OPC relation.
     */
    typedef opc_uint32_t opcRelation;

/**
  Constant which represents an invalid relation.
*/
#define OPC_RELATION_INVALID (-1)

    /**
      Find a relation originating from \c part in \c container with \c relationId and/or \c mimeType.
      If \c part is OPC_PART_INVALID then part represents the root part.
      @param[in] relationId The relationId (e.g. "rId1") or NULL.
      @param[in] mimeType The mimeType or NULL.
      */
    opcRelation opcRelationFind(opcContainer *container, opcPart part, const xmlChar *relationId, const xmlChar *mimeType);

    /**
      Deleted the relation from the container.
      \see opcRelationFind.
      */
    opc_error_t opcRelationDelete(opcContainer *container, opcPart part, const xmlChar *relationId, const xmlChar *mimeType);

    /**
      Returns the first relation.
      The following code will dump all relations:
      \code
        for(opcPart part=opcPartGetFirst(c);OPC_PART_INVALID!=part;part=opcPartGetNext(c, part)) {
           for(opcRelation rel=opcRelationFirst(part, c);
               OPC_PART_INVALID!=rel;
               rel=opcRelationNext(c, rel)) {
               opcPart internal_target=opcRelationGetInternalTarget(c, part, rel);
               const xmlChar *external_target=opcRelationGetExternalTarget(c, part, rel);
               const xmlChar *target=(NULL!=internal_target?internal_target:external_target);
               const xmlChar *prefix=NULL;
               opc_uint32_t counter=-1;
               const xmlChar *type=NULL;
               opcRelationGetInformation(c, part, rel, &prefix, &counter, &type);        
               if (-1==counter) { // no counter after prefix
                  printf("%s;%s;%s;%s\n", part, prefix, target, type);
               } else {
                  printf("%s;%s%i;%s;%s\n", part, prefix, counter, target, type);
               }
           }
        }
      \endcode
      */
    opcRelation opcRelationFirst(opcContainer *container, opcPart part);

    /**
      \see opcRelationFirst
      */
    opcRelation opcRelationNext(opcContainer *container, opcPart part, opcRelation relation);
    
    /**
      Returns the internal target.
      \note To test for an external target use opcRelationGetExternalTarget.
      \see opcRelationGetExternalTarget
      */
    opcPart opcRelationGetInternalTarget(opcContainer *container, opcPart part, opcRelation relation);

    /**
      Returns the external target or NULL if it is an internal target.
      The string is interned. Must not be freed.
      \see opcRelationGetExternalTarget
      */
    const xmlChar *opcRelationGetExternalTarget(opcContainer *container, opcPart part, opcRelation relation);

    /**
      Returns the relations type.
      The string is interned. Must not be freed.
      */
    const xmlChar *opcRelationGetType(opcContainer *container, opcPart part, opcRelation relation);

    /** 
      Get information about a relation.
      \see opcRelationFirst
      */
    void opcRelationGetInformation(opcContainer *container, opcPart part, opcRelation relation, const xmlChar **prefix, opc_uint32_t *counter, const xmlChar **type);

    /** 
      Add a relation to \c container from \c src part to \c dest part with id \c rid and type \c type.
      */
    opc_uint32_t opcRelationAdd(opcContainer *container, opcPart src, const xmlChar *rid, opcPart dest, const xmlChar *type);

    /** 
      Add an external relation to \c container from \c src part to \c target URL with id \c rid and type \c type.
      */
    opc_uint32_t opcRelationAddExternal(opcContainer *container, opcPart src, const xmlChar *rid, const xmlChar *target, const xmlChar *type);

#ifdef __cplusplus
} /* extern "C" */
#endif    

#endif /* OPC_RELATION_H */
