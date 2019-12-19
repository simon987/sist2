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
/**@file config/opc/config.h
 */
#ifndef OPC_CONFIG_H
#define OPC_CONFIG_H

#include <libxml/xmlstring.h>
#include <plib/plib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif   


/**
  Assert expression e is true. Will be removed entirely in release mode.
  \hideinitializer
 */
#define OPC_ASSERT(e) assert(e)

/**
  Assert expression e is true. Expression will be executed in release mode too.
  \hideinitializer
 */
#ifdef NDEBUG
#define OPC_ENSURE(e) (void)(e)
#else
#define OPC_ENSURE(e) assert(e)
#endif


/**
  Constant for boolean true.
  \hideinitializer
 */
#define OPC_TRUE (0==0)

/**
  Constant for boolean false.
  \hideinitializer
 */
#define OPC_FALSE (0==1)

    /** 
      Boolean type.
      \hideinitializer
      */
    typedef pbool_t opc_bool_t;

    /** 
      Type which represents an offset in e.g. a file.
      \hideinitializer
      */
    typedef pofs_t opc_ofs_t;

    /** 
      8-bit unsigned integer.
      \hideinitializer
      */
    typedef puint8_t opc_uint8_t;

    /** 
      16-bit unsigned integer.
      \hideinitializer
      */
    typedef puint16_t opc_uint16_t;

    /** 
      32-bit unsigned integer.
      \hideinitializer
      */
    typedef puint32_t opc_uint32_t;

    /** 
      64-bit unsigned integer.
      \hideinitializer
      */
    typedef puint64_t opc_uint64_t;

    /** 
      8-bit signed integer.
      \hideinitializer
      */
    typedef pint8_t opc_int8_t;

    /** 
      16-bit signed integer.
      \hideinitializer
      */
    typedef pint16_t opc_int16_t;

    /** 
      32-bit signed integer.
      \hideinitializer
      */
    typedef pint32_t opc_int32_t;

    /** 
      64-bit signed integer.
      \hideinitializer
      */
    typedef pint64_t opc_int64_t;

/**
  Default size fo the deflate buffer used by zlib.
  */
#define OPC_DEFLATE_BUFFER_SIZE 4096

/**
  Max system path len.
  */
#define OPC_MAX_PATH 512

    /**
      Error codes for the OPC module.
      */
    typedef enum OPC_ERROR_ENUM {
        OPC_ERROR_NONE,
        OPC_ERROR_STREAM,
        OPC_ERROR_SEEK, // can't seek
        OPC_ERROR_UNSUPPORTED_DATA_DESCRIPTOR,
        OPC_ERROR_UNSUPPORTED_COMPRESSION,
        OPC_ERROR_DEFLATE,
        OPC_ERROR_HEADER,
        OPC_ERROR_MEMORY,
        OPC_ERROR_XML, 
        OPC_ERROR_USER // user triggered an abort
    } opc_error_t;
    
    /**
      Compression options for OPC streams.
      */
    typedef enum OPC_COMPRESSIONOPTION_ENUM {
        OPC_COMPRESSIONOPTION_NONE,
        OPC_COMPRESSIONOPTION_NORMAL,
        OPC_COMPRESSIONOPTION_MAXIMUM,
        OPC_COMPRESSIONOPTION_FAST,
        OPC_COMPRESSIONOPTION_SUPERFAST
    } opcCompressionOption_t;


/**
  Helper for debug logs.
  \hideinitializer
  */
#define opc_logf printf

/**
  Abstraction for memset(m, 0, s).
  \hideinitializer
 */
#define opc_bzero_mem(m,s) memset(m, 0, s)

#ifdef __cplusplus
} /* extern "C" */
#endif  

#endif /* OPC_CONFIG_H */
