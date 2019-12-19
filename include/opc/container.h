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
/** @file opc/container.h

 The container.h module has the fundamental methods for dealing with ZIP-based OPC container. 
 
 OPC container can be opened in READ-ONLY mode, WRITE-ONLY mode, READ/WRITE mode, TEMPLATE mode and TRANSITION mode. 
 The most notable mode is the READ/WRITE mode, which gives you concurrent stream-based READ and WRITE access to a 
 single ZIP-based OPC container. This is achieved without the use of temporary files by taking advantage of the 
 OPC specific “interleave” mode. \see http://standards.iso.org/ittf/PubliclyAvailableStandards/c051459_ISOIEC_29500-2_2008(E).zip
 
 The TEMPLATE mode allows very fast customized "cloning" of ZIP-based OPC container by using "RAW access" to the ZIP streams. 
 The TRANSITION mode is a special version of the TEMPLATE mode, which allows transition-based READ/WRITE access to the 
 ZIP-based OPC container using a temporary file.
 
 */
#include <opc/config.h>
#include <opc/file.h>

#ifndef OPC_CONTAINER_H
#define OPC_CONTAINER_H

#ifdef __cplusplus
extern "C" {
#endif    
    /**
     Handle to an OPC container created by \ref opcContainerOpen.
     \see opcContainerOpen.
     */
    typedef struct OPC_CONTAINER_STRUCT opcContainer;
    
    /**
     Modes for opcContainerOpen();
     \see opcContainerOpen
     */
    typedef enum {
        /**
         Opens the OPC container denoted by \a fileName in READ-ONLY mode. The \a destName parameter must be \a NULL.
         \hideinitializer
         */
        OPC_OPEN_READ_ONLY=0, 
        /**
         Opens the OPC container denoted by \a fileName in WRITE-ONLY mode. The \a destName parameter must be \a NULL.
         \hideinitializer
         */
        OPC_OPEN_WRITE_ONLY=1,
        /**
         Opens the OPC container denoted by \a fileName in READ/WRITE mode. The \a destName parameter must be \a NULL.
         \hideinitializer
         */
        OPC_OPEN_READ_WRITE=2,
        /**
         This mode will open the container denoted by \a fileName in READ-ONLY mode and the container denoted by 
         \a destName in write-only mode. Any modifications will be written to the container denoted by \a destName 
         and the unmodified streams from \a fileName will be written to \a destName on closing.
         \warning Currently not implemented.
         \hideinitializer
         */
        OPC_OPEN_TEMPLATE=3,
        /**
         Like the OPC_OPEN_TEMPLATE mode, but the \a destName will be renamed to the \a fileName on closing. If \a destName 
         is \a NULL, then the name of the temporary file will be generated automatically.
         \warning Currently not implemented.
         \hideinitializer
         */
        OPC_OPEN_TRANSITION=4
    } opcContainerOpenMode; 
    
    /** Modes for opcContainerClose.
     \see opcContainerClose.
     */
    typedef enum {
        /**
         Close the OPC container without any further postprocessing.
         \hideinitializer
         */
        OPC_CLOSE_NOW = 0,
        /**
         Close the OPC container and trim the file by removing unused fragments like e.g. 
         deleted parts.
         \hideinitializer
         */
        OPC_CLOSE_TRIM = 1,
        /**
         Close the OPC container like in \a OPC_CLOSE_TRIM mode, but additionally remove any 
         "interleaved" parts by reordering them.
         \warning Currently not implemented. Same semantic as OPC_CLOSE_TRIM.       
         \hideinitializer
         */
        OPC_CLOSE_DEFRAG = 2
    } opcContainerCloseMode;
    
    /**
     Opens a ZIP-based OPC container.
     @param[in] fileName. For more details see \ref opcContainerOpenMode.
     @param[in] mode. For more details see \ref opcContainerOpenMode.
     @param[in] userContext. Will not be modified by libopc. Can be used to e.g. store the "this" pointer for C++ bindings.
     @param[in] destName. For more details see \ref opcContainerOpenMode.
     @return \a NULL if failed. 
     \see opcContainerOpenMode
     \see opcContainerDump
     */
    opcContainer* opcContainerOpen(const xmlChar *fileName, 
                                   opcContainerOpenMode mode, 
                                   void *userContext, 
                                   const xmlChar *destName);

    /**
     Opens a ZIP-based OPC container from memory.
     @param[in] data. 
     @param[in] data_len.
     @param[in] userContext. Will not be modified by libopc. Can be used to e.g. store the "this" pointer for C++ bindings.
     @param[in] mode. For more details see \ref opcContainerOpenMode.
     @return \a NULL if failed. 
     */
    opcContainer* opcContainerOpenMem(const opc_uint8_t *data, opc_uint32_t data_len,
                                      opcContainerOpenMode mode, 
                                      void *userContext);

    /**
     Opens a ZIP-based OPC container from memory.
     @param[in] ioread. 
     @param[in] iowrite. 
     @param[in] ioclose. 
     @param[in] ioseek. 
     @param[in] iotrim. 
     @param[in] ioflush. 
     @param[in] iocontext. 
     @param[in] file_size. 
     @param[in] userContext. Will not be modified by libopc. Can be used to e.g. store the "this" pointer for C++ bindings.
     @param[in] mode. For more details see \ref opcContainerOpenMode.
     @return \a NULL if failed. 
     */
    opcContainer* opcContainerOpenIO(opcFileReadCallback *ioread,
                                     opcFileWriteCallback *iowrite,
                                     opcFileCloseCallback *ioclose,
                                     opcFileSeekCallback *ioseek,
                                     opcFileTrimCallback *iotrim,
                                     opcFileFlushCallback *ioflush,
                                     void *iocontext,
                                     pofs_t file_size,
                                     opcContainerOpenMode mode, 
                                     void *userContext);
    
    /**
     Close an OPC container.
     @param[in] c. \ref opcContainer openered by \ref opcContainerOpen.
     @param[in] mode. For more information see \ref opcContainerCloseMode.
     @return Non-zero if successful.
     \see opcContainerOpen
     \see opcContainerCloseMode
     */
    opc_error_t opcContainerClose(opcContainer *c, opcContainerCloseMode mode);
    
    /**
     Returns the unmodified user context passed to \ref opcContainerOpen.
     \see opcContainerOpen
     */
    void *opcContainerGetUserContext(opcContainer *c);
    
    /**
     List all types, relations and parts of the container \a c to \a out.
     \par Sample:
     \include opc_dump.c
     */
    opc_error_t opcContainerDump(opcContainer *c, FILE *out);
    
    /**
     Exports the OPC container to "Flat OPC" (http://blogs.msdn.com/b/ericwhite/archive/2008/09/29/the-flat-opc-format.aspx).
     The flat versions of an OPC file are very important when dealing with e.g XSL(T)-based or Javascript-based transformations.
     \see opcContainerFlatImport.
     \todo Implementation needed.
     */
    int opcContainerFlatExport(opcContainer *c, const xmlChar *fileName);
    
    /**
     Imports the flat version of an OPC container. 
     \see opcContainerFlatExport.
     \todo Implementation needed.
     */
    int opcContainerFlatImport(opcContainer *c, const xmlChar *fileName);
    
    /**
     Iterate all types.
     \code
     for(xmlChar *type=opcContentTypeFirst(c);
         NULL!=type;
         type=opcContentTypeNext(c, type)) {
        printf("%s\n", type);
     }
     \endcode
    */
    const xmlChar *opcContentTypeFirst(opcContainer *container);
    
    /**
     \see opcContentTypeNext()
    */
    const xmlChar *opcContentTypeNext(opcContainer *container, const xmlChar *type);

    /**
     Iterate extensions.
     \code
     for(const xmlChar *ext=opcExtensionFirst(c);
         NULL!=ext;
         ext=opcExtensionNext(ext)) {
        printf("%s\n", ext);
     }
     \endcode
    */
    const xmlChar *opcExtensionFirst(opcContainer *container);
    
    /**
     \see opcExtensionFirst()
     */
    const xmlChar *opcExtensionNext(opcContainer *container, const xmlChar *ext);
    
    /**
     Get registered type for extension.
     \see opcExtensionRegister()
     */
    const xmlChar *opcExtensionGetType(opcContainer *container, const xmlChar *ext);

    /**
     Register a mime-type and and extension.
     \see opcExtensionGetType()
     */
    const xmlChar *opcExtensionRegister(opcContainer *container, const xmlChar *ext, const xmlChar *type);


    /**
     Iterator through all relation types of the container:
     \code
     for(xmlChar *type=opcRelationTypeFirst(c);
         NULL!=type;
         type=opcRelationTypeNext(c, type)) {
        printf("%s\n", type);
     }
     \endcode
     */
    const xmlChar *opcRelationTypeFirst(opcContainer *container);

    /**
     \see opcRelationTypeFirst()
    */
    const xmlChar *opcRelationTypeNext(opcContainer *container, const xmlChar *type);


    /**
     Iterator through all relation types of the container:
     \code
     for(xmlChar *target=opcExternalTargetFirst(c);
         NULL!=target;
         type=opcExternalTargetNext(c, target)) {
        printf("%s\n", target);
     }
     \endcode
     */
    const xmlChar *opcExternalTargetFirst(opcContainer *container);

    /**
     \see opcExternalTargetFirst()
    */
    const xmlChar *opcExternalTargetNext(opcContainer *container, const xmlChar *target);


#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_CONTAINER_H */
