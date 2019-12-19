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
/** @file opc/outputstream.h
 
 */
#include <opc/config.h>

#ifndef OPC_OUTPUTSTREAM_H
#define OPC_OUTPUTSTREAM_H

#ifdef __cplusplus
extern "C" {
#endif    
    /**
      Internal type which represents a binary output stream.
      */
    typedef struct OPC_CONTAINER_OUTPUTSTREAM_STRUCT opcContainerOutputStream;

    /** 
      Open the part \c name or writing in \c container with compression \c compression_option.
      \note Make sure the part exists! 
      \see opcPartCreate.
      */
    opcContainerOutputStream* opcContainerCreateOutputStream(opcContainer *container, const xmlChar *name, opcCompressionOption_t compression_option);

    /**
      Write \c buffer_len bytes from \c buffer to \c stream. 
      \return Returns the number of bytes written.
      */
    opc_uint32_t opcContainerWriteOutputStream(opcContainerOutputStream* stream, const opc_uint8_t *buffer, opc_uint32_t buffer_len);

    /**
      Close the \c stream and free all associated resources.
      */
    opc_error_t opcContainerCloseOutputStream(opcContainerOutputStream* stream);
        
#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_OUTPUTSTREAM_H */
