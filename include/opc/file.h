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
/** @file opc/file.h
 The opc module contains the file library functions.
*/
#include <opc/config.h>

#ifndef OPC_FILE_H
#define OPC_FILE_H

#ifdef __cplusplus
extern "C" {
#endif    

/**
  Flag for READ access.
  \hideinitializer
*/
#define OPC_FILE_READ  (1<<0)

/**
  Flag for WRITE access.
  \hideinitializer
*/
#define OPC_FILE_WRITE (1<<1)

/**
  Flag indicates that file will be truncated when opened.
  \hideinitializer
*/
#define OPC_FILE_TRUNC  (1<<2)


    /** 
      Abstraction for see modes.
      */
    typedef enum OPC_FILESEEKMODE_ENUM {
        opcFileSeekSet = SEEK_SET,
        opcFileSeekCur = SEEK_CUR,
        opcFileSeekEnd = SEEK_END
    } opcFileSeekMode;

     /**
      Callback to read a file. E.g. for a FILE * context this can be implemented as
      \code
      static int opcFileRead(void *iocontext, char *buffer, int len) {
          return fread(buffer, sizeof(char), len, (FILE*)iocontext);
      }
      \endcode
      */
    typedef int opcFileReadCallback(void *iocontext, char *buffer, int len);

     /**
      Callback to write a file. E.g. for a FILE * context this can be implemented as
      \code
      static int opcFileWrite(void *iocontext, const char *buffer, int len) {
          return fwrite(buffer, sizeof(char), len, (FILE*)iocontext);
      }
      \endcode
      */
    typedef int opcFileWriteCallback(void *iocontext, const char *buffer, int len);

     /**
      Callback to close a file. E.g. for a FILE * context this can be implemented as
      \code
      static int opcFileClose(void *iocontext) {
          return fclose((FILE*)iocontext);
      }
      \endcode
      */
    typedef int opcFileCloseCallback(void *iocontext);

     /**
      Callback to seek a file. E.g. for a FILE * context this can be implemented as
      \code
      static opc_ofs_t opcFileSeek(void *iocontext, opc_ofs_t ofs) {
          int ret=fseek((FILE*)iocontext, ofs, SEEK_SET);
          if (ret>=0) {
              return ftell((FILE*)iocontext);
          } else {
              return ret;
          }
      }
      \endcode
      */
    typedef opc_ofs_t opcFileSeekCallback(void *iocontext, opc_ofs_t ofs);

     /**
      Callback to trim a file. E.g. for a FILE * context this can be implemented as
      \code
      static int opcFileTrim(void *iocontext, opc_ofs_t new_size) {
      #ifdef WIN32
          return _chsize(fileno((FILE*)iocontext), new_size);
      #else
          return ftruncate(fileno((FILE*)iocontext), new_size);
      #endif
      }
      \endcode
      */
    typedef int opcFileTrimCallback(void *iocontext, opc_ofs_t new_size);

     /**
      Callback to flush a file. E.g. for a FILE * context this can be implemented as
      \code
      static int opcFileFlush(void *iocontext) {
          return fflush((FILE*)iocontext);
      }
      \endcode
      */
    typedef int opcFileFlushCallback(void *iocontext);

    /**
      Represents a state of a file, i.e. file position (buf_pos) and error status (err).
      */
    typedef struct OPC_FILERAWSTATE_STRUCT {
        opc_error_t err;
        opc_ofs_t   buf_pos; // current pos in file
    } opcFileRawState;

    /**
     File IO context.
     */
    typedef struct OPC_IO_STRUCT {
        opcFileReadCallback *_ioread;
        opcFileWriteCallback *_iowrite;
        opcFileCloseCallback *_ioclose;
        opcFileSeekCallback *_ioseek;
        opcFileTrimCallback *_iotrim;
        opcFileFlushCallback *_ioflush;
        void *iocontext;
        int flags;
        opcFileRawState state;
        opc_ofs_t file_size;
    } opcIO_t;

    /**
      Initialize an IO context.
      */
    opc_error_t opcFileInitIO(opcIO_t *io,
                              opcFileReadCallback *ioread,
                              opcFileWriteCallback *iowrite,
                              opcFileCloseCallback *ioclose,
                              opcFileSeekCallback *ioseek,
                              opcFileTrimCallback *iotrim,
                              opcFileFlushCallback *ioflush,
                              void *iocontext,
                              pofs_t file_size,
                              int flags);

    /**
      Initialize an IO context for a file.
      */
    opc_error_t opcFileInitIOFile(opcIO_t *io, const xmlChar *filename, int flags);

    /**
      Initialize an IO for memory.
      \warning Currently supports READ-ONLY file access.
      */
    opc_error_t opcFileInitIOMemory(opcIO_t *io, const opc_uint8_t *data, opc_uint32_t data_len, int flags);

    /**
      Cleanup an IO context, i.e. release all system resources.
      */
    opc_error_t opcFileCleanupIO(opcIO_t *io);

#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_FILE_H */
