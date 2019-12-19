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
/** @file opc/helper.h
  Contains helper functions for the opc module.
*/
#include <opc/config.h>

#ifndef OPC_HELPER_H
#define OPC_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif    


#ifdef __cplusplus
} /* extern "C" */
#endif    

    /**
      Constructs a segment name.
      */
    opc_uint16_t opcHelperAssembleSegmentName(char *out, opc_uint16_t out_size, const xmlChar *name, opc_uint32_t segment_number, opc_uint32_t next_segment_id, opc_bool_t rels_segment, opc_uint16_t *out_max);

    /**
      Splits a filename into the segment informations.
      */
    opc_error_t opcHelperSplitFilename(opc_uint8_t *filename, opc_uint32_t filename_length, opc_uint32_t *segment_number, opc_bool_t *last_segment, opc_bool_t *rel_segment);

#endif /* OPC_HELPER_H */
