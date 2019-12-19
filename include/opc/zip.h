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
/** @file opc/zip.h
  The ZIP file backend of an OPC container.
 */
#include <opc/config.h>
#include <opc/file.h>
#include <opc/container.h>

#ifndef OPC_ZIP_H
#define OPC_ZIP_H

#ifdef __cplusplus
extern "C" {
#endif    

    /**
     Default growth hint of an OPC stream.
     */
    #define OPC_DEFAULT_GROWTH_HINT 512

    /**
     Handle to a ZIP archive.
     \see internal.h
     */
    typedef struct OPC_ZIP_STRUCT opcZip;

    /**
      Handle to a raw ZIP input stream.
     \see internal.h
      */
    typedef struct OPC_ZIPINPUTSTREAM_STRUCT opcZipInputStream;

    /**
      Handle to a raw ZIP output stream.
     \see internal.h
      */
    typedef struct OPC_ZIPOUTPUTSTREAM_STRUCT opcZipOutputStream;

    /**
     Holds all information of a ZIP segment.
     */
    typedef struct OPC_ZIP_SEGMENT_INFO_STRUCT {
        xmlChar name[OPC_MAX_PATH]; 
        opc_uint32_t name_len;
        opc_uint32_t segment_number;
        opc_bool_t   last_segment;
        opc_bool_t   rels_segment;
        opc_uint32_t header_size;
        opc_uint32_t min_header_size;
        opc_uint32_t trailing_bytes;
        opc_uint32_t compressed_size;
        opc_uint32_t uncompressed_size;
        opc_uint16_t bit_flag;
        opc_uint32_t data_crc;
        opc_uint16_t compression_method;
        opc_ofs_t    stream_ofs;
        opc_uint16_t growth_hint;
    } opcZipSegmentInfo_t;

    /**
      \see opcZipLoader
      */
    typedef int opcZipLoaderOpenCallback(void *iocontext);
    /**
      \see opcZipLoader
      */
    typedef int opcZipLoaderSkipCallback(void *iocontext);
    /**
      \see opcZipLoader
      */
    typedef int opcZipLoaderReadCallback(void *iocontext, char *buffer, int len);
    /**
      \see opcZipLoader
      */
    typedef int opcZipLoaderCloseCallback(void *iocontext);

    /**
      \see opcZipLoader
      */
    typedef opc_error_t (opcZipLoaderSegmentCallback_t)(void *iocontext, void *userctx, opcZipSegmentInfo_t *info, opcZipLoaderOpenCallback *open, opcZipLoaderReadCallback *read, opcZipLoaderCloseCallback *close, opcZipLoaderSkipCallback *skip);

    /**
      Walks every segment in a ZIP archive and calls the \c segmentCallback callback method.
      The implementer \c segmentCallback method must then eiher use the passed \c open, \c read and \c close methods
      to read the stream or the passed \c skip methods to skip the stream.
      This method can be used to e.g. read ZIP file in stream mode.
      */
    opc_error_t opcZipLoader(opcIO_t *io, void *userctx, opcZipLoaderSegmentCallback_t *segmentCallback);

    /**
      \see opcZipClose
     */
    typedef opc_error_t (opcZipSegmentReleaseCallback)(opcZip *zip, opc_uint32_t segment_id);

    /** 
     Closes the ZIP archive \c zip and will call \c releaseCallback for every segment to give the implementer a chance
     to free user resources.
     */
    void opcZipClose(opcZip *zip, opcZipSegmentReleaseCallback* releaseCallback);

    /**
      Creates an empty ZIP archive with the given \c io.
      */
    opcZip *opcZipCreate(opcIO_t *io);

    /**
      Commits all buffers and writes the ZIP archives local header directories.
      if \c trim is true then padding bytes will be removed, i.e. the ZIP file size fill be minimalized.
     */
    opc_error_t opcZipCommit(opcZip *zip, opc_bool_t trim);

    /**
      Garbage collection on the passed \c zip archive. This will e.g. make deleted files available as free space.
      */
    opc_error_t opcZipGC(opcZip *zip);

    /**
      Load segment information into \c info.
      If \c rels_segment is -1 then load the info for part with name \c partName.
      Otherwise load the segment information for the ".rels." segment of \c partName.
      \return Returns the segment_id.
      */
    opc_uint32_t opcZipLoadSegment(opcZip *zip, const xmlChar *partName, opc_bool_t rels_segment, opcZipSegmentInfo_t *info);

    /**
      Create a segment with the given parameters.
      \return Returns the segment_id.
      */
    opc_uint32_t opcZipCreateSegment(opcZip *zip, 
                                     const xmlChar *partName, 
                                     opc_bool_t relsSegment, 
                                     opc_uint32_t segment_size, 
                                     opc_uint32_t growth_hint,
                                     opc_uint16_t compression_method,
                                     opc_uint16_t bit_flag);

    /**
      Creates an input stream for the segment with \c segment_id.
      \see opcZipLoadSegment
      \see opcZipCreateSegment
      */
    opcZipInputStream *opcZipOpenInputStream(opcZip *zip, opc_uint32_t segment_id);

    /**
     Free all resources of the input stream.
     */
    opc_error_t opcZipCloseInputStream(opcZip *zip, opcZipInputStream *stream);

    /**
     Read maximal \c buf_len bytes from the input stream into \buf. 
     \return Returns the number of bytes read.
     */
    opc_uint32_t opcZipReadInputStream(opcZip *zip, opcZipInputStream *stream, opc_uint8_t *buf, opc_uint32_t buf_len);


    /**
      Creates an output stream for the segment with \c segment_id.
      If \c *segment_id is -1 then a new segment will be created. 
      Otherwise the segment with \c *segment_id will be overwritten.
     */
    opcZipOutputStream *opcZipCreateOutputStream(opcZip *zip, 
                                             opc_uint32_t *segment_id, 
                                             const xmlChar *partName, 
                                             opc_bool_t relsSegment, 
                                             opc_uint32_t segment_size, 
                                             opc_uint32_t growth_hint,
                                             opc_uint16_t compression_method,
                                             opc_uint16_t bit_flag);

    /**
      Opens an existing ouput stream for reading.
      The \c *segment_id will be set to -1 and reset on opcZipCloseOutputStream.
      \see opcZipCloseOutputStream
     */
    opcZipOutputStream *opcZipOpenOutputStream(opcZip *zip, opc_uint32_t *segment_id);

    /** 
      Will close the stream and free all resources. Additionally the new segment id will be stored in \c *segment_id.
      \see opcZipOpenOutputStream
      */
    opc_error_t opcZipCloseOutputStream(opcZip *zip, opcZipOutputStream *stream, opc_uint32_t *segment_id);

    /**
     Write \c buf_len bytes to \c buf. 
     \return Returns the number of bytes written.
     */
    opc_uint32_t opcZipWriteOutputStream(opcZip *zip, opcZipOutputStream *stream, const opc_uint8_t *buf, opc_uint32_t buf_len);

    /**
     Returns the first segment id or -1.
     Use the following code to iterarte through all segments.
     \code 
     for(opc_uint32_t segment_id=opcZipGetFirstSegmentId(zip);
         -1!=segment_id;
         segment_id=opcZipGetNextSegmentId(zip, segment_id) {
        ...
     }
     \endcode
     \see opcZipGetNextSegmentId
     */
    opc_uint32_t opcZipGetFirstSegmentId(opcZip *zip);

    /**
     Returns the next segment id or -1.
     \see opcZipGetFirstSegmentId
     */
    opc_uint32_t opcZipGetNextSegmentId(opcZip *zip, opc_uint32_t segment_id);

    /**
     Returns info about the given segment id.
     */
    opc_error_t opcZipGetSegmentInfo(opcZip *zip, opc_uint32_t segment_id, const xmlChar **name, opc_bool_t *rels_segment, opc_uint32_t *crc);

    /**
     Marks a given segments as deleted.
     \see opcZipGC
     */
    opc_bool_t opcZipSegmentDelete(opcZip *zip, opc_uint32_t *first_segment, opc_uint32_t *last_segment, opcZipSegmentReleaseCallback* releaseCallback);

#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_ZIP_H */
