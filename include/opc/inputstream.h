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
/** @file opc/inputstream.h
 
 */
#include <opc/config.h>

#ifndef OPC_INPUTSTREAM_H
#define OPC_INPUTSTREAM_H

#ifdef __cplusplus
extern "C" {
#endif    
    /**
      Internal type which represents a binary input stream.
      */
    typedef struct OPC_CONTAINER_INPUTSTREAM_STRUCT opcContainerInputStream;

    /**
      Opens the part \c name of the \c container for reading.
      */
    opcContainerInputStream* opcContainerOpenInputStream(opcContainer *container, const xmlChar *name);

    /**
     Reads maximal \c buffer_len bytes from the input \c stream to \c buffer. 
     \return The number of byes read or "0" in case of an error or end-of-stream.
     */
    opc_uint32_t opcContainerReadInputStream(opcContainerInputStream* stream, opc_uint8_t *buffer, opc_uint32_t buffer_len);

    /**
      Closes the input stream and releases all system resources.
      */
    opc_error_t opcContainerCloseInputStream(opcContainerInputStream* stream);

    /**
      Returns the type of compression used for the stream.
      */
    opcCompressionOption_t opcContainerGetInputStreamCompressionOption(opcContainerInputStream* stream);

#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_INPUTSTREAM_H */
