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
/** @file opc/xmlreader.h
 
 */

#ifndef OPC_XMLREADER_H
#define OPC_XMLREADER_H

#include <opc/config.h>
#include <libxml/xmlreader.h>
#include <mce/textreader.h>


#ifdef __cplusplus
extern "C" {
#endif    

    /** 
      Open an MCE reader for \c partName. Parameters \c URL, \c encoding and \c options will be passed unmodified to 
      http://xmlsoft.org/html/libxml-xmlreader.html#xmlReaderForIO and they can we NULL, NULL, 0.
      \note Make sure the part exists.
      \see opcPartFind
      */
    opc_error_t opcXmlReaderOpen(opcContainer *container, mceTextReader_t *mceTextReader, const xmlChar *partName, const char * URL, const char * encoding, int options);

    /**
      Returns an libxml DOM document. Parameters \c URL, \c encoding and \c options will be passed unmodified to 
      http://xmlsoft.org/html/libxml-parser.html#xmlReadIO and they can we NULL, NULL, 0.
      \note Make sure the part exists.
      \see opcPartFind
      */
    xmlDocPtr opcXmlReaderReadDoc(opcContainer *container, const xmlChar *partName, const char * URL, const char * encoding, int options);

#ifdef __cplusplus
} /* extern "C" */
#endif    
        
#endif /* OPC_XMLREADER_H */
