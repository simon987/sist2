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
/** @file mce/helper.h
Helper functions needed by mce/textreader.h and mce/textwriter.h to implement MCE:
- mceQNameLevelAdd(), mceQNameLevelLookup() and mceQNameLevelCleanup() maintain a set of mceQNameLevel_t tuples.
- mceQNameLevelPush() and mceQNameLevelPopIfMatch() maintain a stack of mceQNameLevel_t tuples.
- mceCtxInit(), mceCtxCleanup() and mceCtxUnderstandsNamespace() manage a context which holds all information needed to do MCE proprocessing.
 */
#include <mce/config.h>

#ifndef MCE_HELPER_H
#define MCE_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

    /**
      Tiple (ns, ln, level).
    */
    typedef struct MCE_QNAME_LEVEL {
        xmlChar *ns;
        xmlChar *ln;
        puint32_t level;
        puint32_t flag; // used by mceTextWriter
    } mceQNameLevel_t;

    /**
     */
    typedef enum MCE_SKIP_STATE_ENUM {
        MCE_SKIP_STATE_IGNORE,
        MCE_SKIP_STATE_ALTERNATE_CONTENT,
        MCE_SKIP_STATE_CHOICE_MATCHED
    } mceSkipState_t;

    /**
     Represents an intervall of levels which are "skipped" i.e. ignored.
     */
    typedef struct MCE_SKIP_ITEM {
        puint32_t level_start;
        puint32_t level_end;
        mceSkipState_t state;
    } mceSkipItem_t;

    /**
      Either represents a set of (ns, ln, level) triples.
    */
    typedef struct MCE_QNAME_LEVEL_SET {
        mceQNameLevel_t *list_array;
        puint32_t list_items;
        puint32_t max_level;
    } mceQNameLevelSet_t;

    /**
     The skip stack.
     */
    typedef struct MCE_SKIP_STACK {
        mceSkipItem_t *stack_array;
        puint32_t stack_items;
    } mceSkipStack_t;


    typedef enum MCE_ERROR_ENUM {
        MCE_ERROR_NONE,
        MCE_ERROR_XML,
        MCE_ERROR_MUST_UNDERSTAND,
        MCE_ERROR_VALIDATION,
        MCE_ERROR_MEMORY
    } mceError_t;

    /**
      Holds all information to do MCE preprocessing.
    */
    typedef struct MCE_CONTEXT {
        mceQNameLevelSet_t ignorable_set;
        mceQNameLevelSet_t understands_set;
        mceQNameLevelSet_t processcontent_set;
        mceQNameLevelSet_t suspended_set;
#if (MCE_NAMESPACE_SUBSUMPTION_ENABLED)
        mceQNameLevelSet_t subsume_namespace_set;
        mceQNameLevelSet_t subsume_exclude_set;
        mceQNameLevelSet_t subsume_prefix_set;
#endif
        mceSkipStack_t skip_stack;
        mceError_t error;
        pbool_t mce_disabled;        
        puint32_t suspended_level;
    } mceCtx_t;

    /**
      Add a new tiple (ns, ln, level) to the triple set \c qname_level_set.
      The \c ns_sub string is optional and will not be touched.
    */
    pbool_t mceQNameLevelAdd(mceQNameLevelSet_t *qname_level_set, const xmlChar *ns, const xmlChar *ln, puint32_t level);

    /**
      Lookup a tiple (ns, ln, level) via \c ns and \c ln. If \c ignore_ln is PTRUE then the first tiple matching \c ns will be returned.
    */
    mceQNameLevel_t* mceQNameLevelLookup(mceQNameLevelSet_t *qname_level_set, const xmlChar *ns, const xmlChar *ln, pbool_t ignore_ln);

    /**
      Remove all triples (ns, ln, level) where the level greater or equal to \c level.
    */
    pbool_t mceQNameLevelCleanup(mceQNameLevelSet_t *qname_level_set, puint32_t level);

    /**
      Push a new skip intervall (level_start, level_end, state) on the stack \c skip_stack.
    */
    pbool_t mceSkipStackPush(mceSkipStack_t *skip_stack, puint32_t level_start, puint32_t level_end, mceSkipState_t state);

    /**
      Pop the intervall (ns, ln, level) from the stack \c qname_level_array.
    */
    void mceSkipStackPop(mceSkipStack_t *skip_stack);

    /**
     Returns top item or NULL.
     */
    mceSkipItem_t *mceSkipStackTop(mceSkipStack_t *skip_stack);

    /**
     Returns TRUE, if the \c level is in the top skip intervall.
     */
    pbool_t mceSkipStackSkip(mceSkipStack_t *skip_stack, puint32_t level);

    /**
      Initialize the mceCtx_t \c ctx.
    */
    pbool_t mceCtxInit(mceCtx_t *ctx);

    /**
      Cleanup, i.e. release all resourced from the mceCtx_t \c ctx.
    */
    pbool_t mceCtxCleanup(mceCtx_t *ctx);

    /**
      Register the namespace \ns in \c ctx.
    */
    pbool_t mceCtxUnderstandsNamespace(mceCtx_t *ctx, const xmlChar *ns);

    /**
     Register the namespace \ns in \c ctx.
     */
    pbool_t mceCtxSuspendProcessing(mceCtx_t *ctx, const xmlChar *ns, const xmlChar *ln);
    


#if (MCE_NAMESPACE_SUBSUMPTION_ENABLED)
    /**
    Subsume namespace \c ns_new with \c ns_old.
     */
    pbool_t mceCtxSubsumeNamespace(mceCtx_t *ctx, const xmlChar *prefix_new, const xmlChar *ns_new, const xmlChar *ns_old);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MCE_HELPER_H */
