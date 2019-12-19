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
/** @file mce/textreader.h
 
 */
#ifndef MCE_TEXTREADER_H
#define MCE_TEXTREADER_H

#ifdef __cplusplus
extern "C" {
#endif

    /**
      A handle to an MCE-aware libxml2 xmlTextReader.
    */
    typedef struct MCE_TEXTREADER mceTextReader_t;

#ifdef __cplusplus
} /* extern "C" */
#endif


#include <mce/config.h>
#include <opc/opc.h>
#include <mce/helper.h>
#include <libxml/xmlwriter.h>


#ifdef __cplusplus
extern "C" {
#endif

    struct MCE_TEXTREADER {
        xmlTextReaderPtr reader;
        mceCtx_t mceCtx;
    };

    /**
      Wrapper around an libxml2 xmlTextReaderRead function.
      \see http://xmlsoft.org/html/libxml-xmlreader.html#xmlTextReaderRead
    */
    int mceTextReaderRead(mceTextReader_t *mceTextReader);

    /**
      Wrapper around a libxml2 xmlTextReaderNext function.
      \see http://xmlsoft.org/html/libxml-xmlreader.html#xmlTextReaderNext
    */
    int mceTextReaderNext(mceTextReader_t *mceTextReader);

    /** 
      Creates an mceTextReader from an XmlTextReader. 
      \code
      mceTextReader reader;
      mceTextReaderInit(&reader, xmlNewTextReaderFilename("sample.xml"));
      // reader is ready to use.
      mceTextReaderCleanup(&reader);
      \endcode
      \see http://xmlsoft.org/html/libxml-xmlreader.html#xmlNewTextReaderFilename
    */
    int mceTextReaderInit(mceTextReader_t *mceTextReader, xmlTextReaderPtr reader);

    /**
      Cleanup MCE reader, i.e. free all resources. Also calls xmlTextReaderClose and xmlFreeTextReader.
      \see http://xmlsoft.org/html/libxml-xmlreader.html#xmlTextReaderClose
      \see http://xmlsoft.org/html/libxml-xmlreader.html#xmlFreeTextReader
    */
    int mceTextReaderCleanup(mceTextReader_t *mceTextReader);

    /** 
      Reads all events \c mceTextReader and pipes them to \writer.
      \code
      mceTextReader reader;
      mceTextReaderInit(&reader, xmlNewTextReaderFilename("sample.xml"));
      mceTextReaderUnderstandsNamespace(&reader, _X("http://myextension"));
      xmlTextWriterPtr writer=xmlNewTextWriterFilename("out.xml", 0);
      mceTextReaderDump(&reader, writer, P_FALSE);
      xmlFreeTextWriter(writer);
      mceTextReaderCleanup(&reader);
      \endcode
      */
    int mceTextReaderDump(mceTextReader_t *mceTextReader, xmlTextWriter *writer, pbool_t fragment);

    /**
      Registers an MCE namespace.
      \see mceTextReaderDump()
      */
    int mceTextReaderUnderstandsNamespace(mceTextReader_t *mceTextReader, const xmlChar *ns);

    /**
     Disable MCE processing.
     \return Returns old value.
     */
    pbool_t mceTextReaderDisableMCE(mceTextReader_t *mceTextReader, pbool_t flag);


    /**
     Signal an error to the MCE processor.
     */
    void mceRaiseError(xmlTextReader *reader, mceCtx_t *ctx, mceError_t error, const xmlChar *str, ...);

    /**
        Internal function which does the MCE postprocessing. E.g. mceTextReaderRead() is implemented as
        \code
        mceTextReaderPostprocess(mceTextReader->reader, &mceTextReader->mceCtx, xmlTextReaderRead(mceTextReader->reader))
        \endcode
        This function is exposed to make existing libxm2 xmlTextReader MCE aware.
    */
    int mceTextReaderPostprocess(xmlTextReader *reader, mceCtx_t *ctx, int ret);

    /**
     Get the error code.
     */
    mceError_t mceTextReaderGetError(mceTextReader_t *mceTextReader);

/**
 Helper macro to declare a start/end document block in a declarative way:
 \code
  mce_start_document(reader) {
  } mce_end_document(reader);
  \endcode
  \hideinitializer
*/
#define mce_start_document(_reader_) \
    if (NULL!=(_reader_)) {            \
        mceTextReaderRead(_reader_); \
        if (0)                     

/**
  \see mce_start_document.
  \hideinitializer
*/
#define mce_end_document(_reader_)   \
    } /* if (NULL!=reader) */        \


/**
  Container for mce_start_element and mce_start_attribute declarations.
  \see mce_match_element
  \see mce_match_attribute
  \hideinitializer
  */
#define mce_start_choice(_reader_)  \
    if (0)                          

/**
  \see mce_start_choice
  \hideinitializer
  */
#define mce_end_choice(_reader_) 


/**
  Skips the attributes. 
  \see mce_match_element.
  \hideinitializer
*/
#define mce_skip_attributes(_reader_) \
    mce_start_attributes(_reader_) {  \
    } mce_end_attributes(_reader_);   


/**
  Skips the attributes. 
  \see mce_match_attribute.
  \hideinitializer
*/
#define mce_skip_children(_reader_) \
    mce_start_children(_reader_) {  \
    } mce_end_children(_reader_);   

/**
  \see mce_start_element.
  \hideinitializer
*/
#define mce_start_children(_reader_)                  \
if (!xmlTextReaderIsEmptyElement((_reader_)->reader)) { \
    mceTextReaderRead(_reader_); do {                 \
        if (0)                                        

/**
  \see mce_start_element.
  \hideinitializer
*/
#define mce_end_children(_reader_)                                                      \
        else {                                                                          \
            if (XML_READER_TYPE_END_ELEMENT!=xmlTextReaderNodeType((_reader_)->reader)) { \
                mceTextReaderNext(_reader_); /*skip unhandled element */                \
            }                                                                           \
        }                                                                               \
    } while(XML_READER_TYPE_END_ELEMENT!=xmlTextReaderNodeType((_reader_)->reader) &&     \
            XML_READER_TYPE_NONE!=xmlTextReaderNodeType((_reader_)->reader));             \
} /* if (!xmlTextReaderIsEmptyElement(reader->reader)) */                               


/**
  Helper macro to match an element. Usefull for calling code in a seperate function:

  \code
  void handleElement(reader) {
    mce_start_choice(reader) {
        mce_start_element(reader, _X("ns"), _X("element")) {
            
        } mce_end_element(reader)
    } mce_end_choice(reader);
  }

  void parse(reader) {
    mce_start_document(reader) {
      mce_start_element(reader, _X("ns"), _X("ln")) {
        mce_skip_attributes(reader);
        mce_start_children(reader) {
           mce_match_element(reader, _X("ns"), _X("element")) {
             handleElement(reader);
           }
        } mce_end_children(reader);
      } mce_end_element();
    } mce_end_document(reader);
  }
  \endcode
  \hideinitializer
*/
#define mce_match_element(_reader_, ns, ln)                                                       \
    } else if (XML_READER_TYPE_ELEMENT==xmlTextReaderNodeType((_reader_)->reader)                 \
            && (NULL==ns || 0==xmlStrcmp(ns, xmlTextReaderConstNamespaceUri((_reader_)->reader))) \
            && (NULL==ln || 0==xmlStrcmp(ln, xmlTextReaderConstLocalName((_reader_)->reader)))) { 


/**
 Helper macro to declare a element block in a declarative way:
 \code
  mce_start_element(reader) {
    mce_start_attributes(reader) {
      mce_start_attribute(reader, _X("ns"), _X("lnA")) {
         // code for handling lnA.
      } mce_end_attribute(reader);
      mce_start_attribute(reader, _X("ns"), _X("lnB")) {
         // code for handling lnB.
      } mce_end_attribute(reader);
    } mce_end_attributes(reader);
    mce_start_children(reader) {
        mce_start_element(reader, _X("ns"), _X("lnA")) {
         // code for handling lnA.
        } mce_end_element(reader);
        mce_start_element(reader, _X("ns"), _X("lnB")) {
         // code for handling lnB.
        } mce_end_element(reader);
        mce_start_text(reader) {
         // code for handling text.
        } mce_end_text(reader);
    } mce_end_children(reader);
  } mce_end_element(reader);
  \endcode
  \hideinitializer
*/
#define mce_start_element(_reader_, ns, ln) \
    mce_match_element(_reader_, ns, ln)     

/**
  \see mce_start_element.
  \hideinitializer
*/
#define mce_end_element(_reader_) \
    mceTextReaderNext(_reader_)   

/**
  Matches #TEXT without consuming it.
  \hideinitializer
*/
#define mce_match_text(_reader_)                                                                   \
    } else if (XML_READER_TYPE_TEXT==xmlTextReaderNodeType((_reader_)->reader)                     \
            || XML_READER_TYPE_SIGNIFICANT_WHITESPACE==xmlTextReaderNodeType((_reader_)->reader)) {


/**
  \see mce_start_element.
  \hideinitializer
*/
#define mce_start_text(_reader_) \
    mce_match_text(_reader_)      

/**
  \see mce_start_element.
  \hideinitializer
*/
#define mce_end_text(_reader_) \
    mceTextReaderNext(_reader_)

/**
  \see mce_start_element.
  \hideinitializer
*/
#define mce_start_attributes(_reader_)                            \
    if (1==xmlTextReaderMoveToFirstAttribute((_reader_)->reader)) { \
        do {                                                      \
            if (0)                                                

/**
  \see mce_start_element.
  \hideinitializer
*/
#define mce_end_attributes(_reader_)                                    \
            else { /* skipped attribute */ }                            \
        } while(1==xmlTextReaderMoveToNextAttribute((_reader_)->reader)); \
    xmlTextReaderMoveToElement((_reader_)->reader); }                     

/**
  Helper macro to match an attribute. Usefull for calling code in a seperate function:

  \code
  void handleA(reader) {
    mce_start_choice(reader) {
        mce_start_attribute(reader, _X("ns"), _X("attr")) {

        } mce_end_attribute(reader);
    } mce_end_choice(reader);
  }

  void parse(reader) {
    mce_start_document(reader) {
      mce_start_element(reader, _X("ns"), _X("ln")) {
        mce_start_attributes(reader) {
           mce_match_attribute(reader, _X("ns"), _X("attr")) {
             handleA(reader);
           }
        } mce_end_attributes(reader);
        mce_skip_children(reader);
      } mce_end_element();
    } mce_end_document(reader);
  }
  \endcode
  \hideinitializer
*/
#define mce_match_attribute(_reader_, ns, ln)                                                   \
    } else if ((NULL==ns || 0==xmlStrcmp(ns, xmlTextReaderConstNamespaceUri((_reader_)->reader))) \
            && (NULL==ln || 0==xmlStrcmp(ln, xmlTextReaderConstLocalName((_reader_)->reader)))) { 

/**
  \see mce_start_element.
  \hideinitializer
*/
#define mce_start_attribute(_reader_, ns, ln) \
    mce_match_attribute(_reader_, ns, ln) 

/**
  \see mce_start_element.
  \hideinitializer
*/
#define mce_end_attribute(_reader_)


/**
  Error handling for MCE parsers.
  \code
   mce_start_element(&reader, NULL, _X("Default")) {
       const xmlChar *ext=NULL;
       const xmlChar *type=NULL;
       mce_start_attributes(&reader) {
           mce_start_attribute(&reader, NULL, _X("Extension")) {
               ext=xmlTextReaderConstValue(reader.reader);
           } mce_end_attribute(&reader);
           mce_start_attribute(&reader, NULL, _X("ContentType")) {
               type=xmlTextReaderConstValue(reader.reader);
           } mce_end_attribute(&reader);
       } mce_end_attributes(&reader);
       mce_error_guard_start(&reader) {
           mce_error(&reader, NULL==ext || ext[0]==0, MCE_ERROR_VALIDATION, "Missing @Extension attribute!");
           mce_error(&reader, NULL==type || type[0]==0, MCE_ERROR_VALIDATION, "Missing @ContentType attribute!");
           opcContainerType *ct=insertType(c, type, OPC_TRUE);
           mce_error(&reader, NULL==ct, MCE_ERROR_MEMORY, NULL);
           opcContainerExtension *ce=opcContainerInsertExtension(c, ext, OPC_TRUE);
           mce_error(&reader, NULL==ce, MCE_ERROR_MEMORY, NULL);
           mce_errorf(&reader, NULL!=ce->type && 0!=xmlStrcmp(ce->type, type), MCE_ERROR_VALIDATION, "Extension \"%s\" is mapped to type \"%s\" as well as \"%s\"", ext, type, ce->type);
           ce->type=ct->type;
       } mce_error_guard_end(&reader);
       mce_skip_children(&reader);
   } mce_end_element(&reader);
  \endcode
  \hideinitializer
*/
#define mce_error_guard_start(_reader_) if (MCE_ERROR_NONE==(_reader_)->mceCtx.error) do {

/**
  \see mce_error_guard_start
  \hideinitializer
*/
#define mce_error_guard_end(_reader_)  } while(0)

/**
  Signal an error if guard if false.
  \hideinitializer
*/
#define mce_error(_reader_, guard, err, msg) if (guard) { (_reader_)->mceCtx.error=(err); fprintf(stderr, (NULL!=msg?msg:#err));  continue; }

/**
  Signal an error if guard if false.
  \hideinitializer
*/
#if defined(__GNUC__)
#define mce_errorf(_reader_, guard, err, msg, ...) if (guard) { mceRaiseError((_reader_)->reader, &(_reader_)->mceCtx, err, _X((NULL!=msg?msg:#err)), ##__VA_ARGS__ );  continue; }
#else
#define mce_errorf(_reader_, guard, err, msg, ...) if (guard) { mceRaiseError((_reader_)->reader, &(_reader_)->mceCtx, err, _X((NULL!=msg?msg:#err)), __VA_ARGS__ );  continue; }
#endif

/**
  Only issues the error when in "strict mode".
  \hideinitializer
*/
#define mce_error_strict mce_error

/**
  \see mce_error_strict
  \hideinitializer
*/
#define mce_error_strictf mce_errorf


/**
  Marker for a MCE defintion.
  \hideinitializer
*/
#define mce_def

/**
  Marker for a MCE reference.
  \hideinitializer
*/
#define mce_ref(r) (r)


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MCE_TEXTREADER_H */
