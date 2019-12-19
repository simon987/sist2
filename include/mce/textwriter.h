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
/** @file mce/textwriter.h

*/
#include <mce/config.h>
#include <libxml/xmlwriter.h>
#include <mce/helper.h>

#ifndef MCE_TEXTWRITER_H
#define MCE_TEXTWRITER_H

#ifdef __cplusplus
extern "C" {
#endif    

/**
  Default flags for an MCE namespace declaration.
  */
#define MCE_DEFAULT 0x0

/**
  Flags MCE namespace declaration "ignorable".
  */
#define MCE_IGNORABLE 0x1

/**
  Flags MCE namespace declaration "must understand".
  */
#define MCE_MUSTUNDERSTAND 0x2

    /**
      The MCE text writer context.
      */
    typedef struct MCE_TEXTWRITER_STRUCT mceTextWriter;

    /**
      Create a new MCE text writer.
      \see http://xmlsoft.org/html/libxml-xmlIO.html#xmlOutputBufferCreateIO
      \see http://xmlsoft.org/html/libxml-xmlwriter.html#xmlNewTextWriter
      */
    mceTextWriter *mceTextWriterCreateIO(xmlOutputWriteCallback iowrite, xmlOutputCloseCallback  ioclose, void *ioctx, xmlCharEncodingHandlerPtr encoder);

    /**
      Helper which create a new MCE text writer for a FILE handle.
      */
    mceTextWriter *mceNewTextWriterFile(FILE *file);

    /**
      Free all resources for \w.
      */
    int mceTextWriterFree(mceTextWriter *w);

    /**
      \see http://xmlsoft.org/html/libxml-xmlwriter.html#xmlTextWriterStartDocument
      */
    int mceTextWriterStartDocument(mceTextWriter *w);

    /**
      \see http://xmlsoft.org/html/libxml-xmlwriter.html#xmlTextWriterEndDocument
      */
    int mceTextWriterEndDocument(mceTextWriter *w);

    /**
      Start a new XML element. If ns==NULL then there is no namespace and ""==ns means the default namespace.
      \see http://xmlsoft.org/html/libxml-xmlwriter.html#xmlTextWriterStartElement
      \see http://xmlsoft.org/html/libxml-xmlwriter.html#xmlTextWriterStartElementNS
      */
    int mceTextWriterStartElement(mceTextWriter *w, const xmlChar *ns, const xmlChar *ln);

    /**
      \see http://xmlsoft.org/html/libxml-xmlwriter.html#xmlTextWriterEndElement
      */
    int mceTextWriterEndElement(mceTextWriter *w, const xmlChar *ns, const xmlChar *ln);

    /**
      \see http://xmlsoft.org/html/libxml-xmlwriter.html#xmlTextWriterWriteString
      */
    int mceTextWriterWriteString(mceTextWriter *w, const xmlChar *content);

    /**
      Register a namespace. Must be called before mceTextWriterStartElement.
      \see MCE_DEFAULT
      \see MCE_IGNORABLE
      \see MCE_MUSTUNDERSTAND
      */
    const xmlChar *mceTextWriterRegisterNamespace(mceTextWriter *w, const xmlChar *ns, const xmlChar *prefix, int flags);

    /**
      Register qname (ns, ln) as a "process content" element wrt. MCE. Must be called before mceTextWriterStartElement.
      */
    int mceTextWriterProcessContent(mceTextWriter *w, const xmlChar *ns, const xmlChar *ln);

    /**
      Writes a formatted attribute.
      \see http://xmlsoft.org/html/libxml-xmlwriter.html#xmlTextWriterWriteFormatAttribute
      */
    int mceTextWriterAttributeF(mceTextWriter *w, const xmlChar *ns, const xmlChar *ln, const char *value, ...);

    /**
      Starts an MCE alternate content section.
      */
    int mceTextWriterStartAlternateContent(mceTextWriter *w);

    /**
      Ends an MCE alternate content section.
      */
    int mceTextWriterEndAlternateContent(mceTextWriter *w);

    /**
      Start an MCE choice.
      */
    int mceTextWriterStartChoice(mceTextWriter *w, const xmlChar *ns);

    /**
      Ends an MCE choice.
      */
    int mceTextWriterEndChoice(mceTextWriter *w);

    /**
      Start an MCE fallback.
      */
    int mceTextWriterStartFallback(mceTextWriter *w);

    /**
      Ends an MCE fallback.
      */
    int mceTextWriterEndFallback(mceTextWriter *w);


    /**
      Returns the underlying xmlTextWriter.
      */
    xmlTextWriterPtr mceTextWriterIntern(mceTextWriter *w);

    /**
      Helper which create a new xmlTextWriterPtr for a FILE handle.
      */
    xmlTextWriterPtr xmlNewTextWriterFile(FILE *file);


#ifdef __cplusplus
} /* extern "C" */
#endif    

#endif /* MCE_TEXTWRITER_H */
