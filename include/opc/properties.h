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
/** @file opc/properties.h
 
 */
#include <opc/config.h>
#include <opc/container.h>

#ifndef OPC_PROPERTIES_H
#define OPC_PROPERTIES_H

#ifdef __cplusplus
extern "C" {
#endif    

    /** 
      Represents a simple Dublin Core type.
      */
    typedef struct OPC_DC_SIMPLE_TYPE {
        xmlChar *str;
        xmlChar *lang;
    } opcDCSimpleType_t;

    /** 
      Represents the core properties of an OPC container.
      */
    typedef struct OPC_PROPERTIES_STRUCT {
        xmlChar *category;                /* xsd:string     */
        xmlChar *contentStatus;           /* xsd:string     */
        xmlChar *created;                 /* dc:date        */
        opcDCSimpleType_t creator;        /* dc:any         */
        opcDCSimpleType_t description;    /* dc:any         */
        opcDCSimpleType_t identifier;     /* dc:any         */
        opcDCSimpleType_t *keyword_array; /* cp:CT_Keywords */
        opc_uint32_t keyword_items;
        opcDCSimpleType_t language;       /* dc:any         */
        xmlChar *lastModifiedBy;          /* xsd:string     */
        xmlChar *lastPrinted;             /* xsd:dateTime   */
        xmlChar *modified;                /* dc:date        */
        xmlChar *revision;                /* xsd:string     */
        opcDCSimpleType_t subject;        /* dc:any         */
        opcDCSimpleType_t title;          /* dc:any         */
        xmlChar *version;                 /* xsd:string     */
    } opcProperties_t;

    /**
      Initialize the core properties \c cp.
      \see opcCorePropertiesSetString
      */
    opc_error_t opcCorePropertiesInit(opcProperties_t *cp);

    /**
      Cleanup the core properties \c cp, i.e. release all resources.
      \see opcCorePropertiesSetString
      */
    opc_error_t opcCorePropertiesCleanup(opcProperties_t *cp);

    /**
      Rease the core properties \c cp from the container \c.
      */
    opc_error_t opcCorePropertiesRead(opcProperties_t *cp, opcContainer *c);


    /**
      Write/Update the core properties \c cp in the container \c.
      */
    opc_error_t opcCorePropertiesWrite(opcProperties_t *cp, opcContainer *c);

    /**
      Update a string in the core properties the right way.
      \code
      opcProperties_t cp;
      opcCorePropertiesInit(&cp);
      opcCorePropertiesSetString(&cp.revision, "1");
      opcCorePropertiesSetStringLang(&cp.creator, "Florian Reuter", NULL);
      opcCorePropertiesCleanup(&cp);
      \endcode
      */
    opc_error_t opcCorePropertiesSetString(xmlChar **prop, const xmlChar *str);

    /** 
      Update a core properties the right way.
      \see opcCorePropertiesSetString
      */
    opc_error_t opcCorePropertiesSetStringLang(opcDCSimpleType_t *prop, const xmlChar *str, const xmlChar *lang);

#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_PROPERTIES_H */
