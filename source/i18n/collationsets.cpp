/*
*******************************************************************************
* Copyright (C) 2013, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* collationsets.cpp
*
* created on: 2013feb09
* created by: Markus W. Scherer
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucharstrie.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/ustringtrie.h"
#include "collation.h"
#include "collationdata.h"
#include "collationsets.h"
#include "normalizer2impl.h"
#include "uassert.h"
#include "utrie2.h"

// TODO: This code is untested. Test & debug!

U_NAMESPACE_BEGIN

U_CDECL_BEGIN

static UBool U_CALLCONV
enumTailoredRange(const void *context, UChar32 start, UChar32 end, uint32_t ce32) {
    if(ce32 == Collation::MIN_SPECIAL_CE32) {
        return TRUE;  // fallback to base, not tailored
    }
    TailoredSet *ts = (TailoredSet *)context;
    ts->handleCE32(start, end, ce32);
    return U_SUCCESS(ts->errorCode);
}

U_CDECL_END

void
TailoredSet::forData(const CollationData *d, UErrorCode &ec) {
    if(U_FAILURE(ec)) { return; }
    errorCode = ec;  // Preserve info & warning codes.
    data = d;
    utrie2_enum(data->trie, NULL, enumTailoredRange, this);
    ec = errorCode;
}

void
TailoredSet::handleCE32(UChar32 start, UChar32 end, uint32_t ce32) {
    U_ASSERT(ce32 != Collation::MIN_SPECIAL_CE32);
    if(Collation::isSpecialCE32(ce32)) {
        ce32 = data->getIndirectCE32(ce32);
        if(ce32 == Collation::MIN_SPECIAL_CE32) {
            return;
        }
    }
    do {
        uint32_t baseCE32 = baseData->getFinalCE32(baseData->getCE32(start));
        // Do not just continue if ce32 == baseCE32 because
        // contractions and expansions in different data objects
        // normally differ even if they have the same data offsets.
        if(Collation::isSpecialCE32(ce32) || Collation::isSpecialCE32(baseCE32)) {
            compare(start, ce32, baseCE32);
        } else {
            // fastpath
            if(ce32 != baseCE32) {
                tailored->add(start);
            }
        }
    } while(++start <= end);
}

void
TailoredSet::compare(UChar32 c, uint32_t ce32, uint32_t baseCE32) {
    if(Collation::isPrefixCE32(ce32)) {
        const UChar *p = data->contexts + Collation::getPrefixIndex(ce32);
        ce32 = data->getFinalCE32(((uint32_t)p[0] << 16) | p[1]);
        if(Collation::isPrefixCE32(baseCE32)) {
            const UChar *q = baseData->contexts + Collation::getPrefixIndex(baseCE32);
            baseCE32 = baseData->getFinalCE32(((uint32_t)q[0] << 16) | q[1]);
            comparePrefixes(c, p + 2, q + 2);
        } else {
            addPrefixes(data, c, p + 2);
        }
    } else if(Collation::isPrefixCE32(baseCE32)) {
        const UChar *q = baseData->contexts + Collation::getPrefixIndex(baseCE32);
        baseCE32 = baseData->getFinalCE32(((uint32_t)q[0] << 16) | q[1]);
        addPrefixes(baseData, c, q + 2);
    }

    if(Collation::isContractionCE32(ce32)) {
        const UChar *p = data->contexts + Collation::getContractionIndex(ce32);
        ce32 = data->getFinalCE32(((uint32_t)p[0] << 16) | p[1]);
        if(Collation::isContractionCE32(baseCE32)) {
            const UChar *q = baseData->contexts + Collation::getContractionIndex(baseCE32);
            baseCE32 = baseData->getFinalCE32(((uint32_t)q[0] << 16) | q[1]);
            compareContractions(c, p + 2, q + 2);
        } else {
            addContractions(c, p + 2);
        }
    } else if(Collation::isContractionCE32(baseCE32)) {
        const UChar *q = baseData->contexts + Collation::getContractionIndex(baseCE32);
        baseCE32 = baseData->getFinalCE32(((uint32_t)q[0] << 16) | q[1]);
        addContractions(c, q + 2);
    }

    int32_t tag;
    if(Collation::isSpecialCE32(ce32)) {
        tag = Collation::getSpecialCE32Tag(ce32);
        // Currently, the tailoring data builder does not write offset tags.
        // They might be useful for saving space,
        // but they would complicate the builder,
        // and in tailorings we assume that performance of tailored characters is more important.
        U_ASSERT(tag != Collation::OFFSET_TAG);
    } else {
        tag = -1;
    }
    int32_t baseTag;
    if(Collation::isSpecialCE32(baseCE32)) {
        baseTag = Collation::getSpecialCE32Tag(baseCE32);
    } else {
        baseTag = -1;
    }

    // The contraction default CE32 might be another contraction CE32.
    // This is the case if it's the same as the default CE32 of the parent prefix data.
    // The parent prefix default CE32's are compared in a different code path.
    U_ASSERT((tag == Collation::CONTRACTION_TAG) == (baseTag == Collation::CONTRACTION_TAG));
    if(tag == Collation::CONTRACTION_TAG) {
        U_ASSERT(prefix != NULL);
        return;
    }

    U_ASSERT(tag != Collation::PREFIX_TAG);

    // Non-contextual mappings, expansions, etc.
    if(baseTag == Collation::OFFSET_TAG) {
        // We might be comparing a tailoring CE which is a copy of
        // a base offset-tag CE, via the [optimize [set]] syntax
        // or when a single-character mapping was copied for tailored contractions.
        // Offset tags always result in long-primary CEs,
        // with common secondary/tertiary weights.
        if(!Collation::isLongPrimaryCE32(ce32)) {
            add(c);
            return;
        }
        int64_t dataCE = baseData->ces[Collation::getOffsetIndex(baseCE32)];
        uint32_t p = Collation::getThreeBytePrimaryForOffsetData(c, dataCE);
        if(Collation::primaryFromLongPrimaryCE32(ce32) != p) {
            add(c);
            return;
        }
    }

    if(tag != baseTag) {
        add(c);
        return;
    }

    if(tag == Collation::EXPANSION32_TAG) {
        const uint32_t *ce32s = data->ce32s + Collation::getExpansionIndex(ce32);
        int32_t length = Collation::getExpansionLength(ce32);
        if(length == 0) { length = (int32_t)*ce32s++; }

        const uint32_t *baseCE32s = baseData->ce32s + Collation::getExpansionIndex(baseCE32);
        int32_t baseLength = Collation::getExpansionLength(baseCE32);
        if(baseLength == 0) { baseLength = (int32_t)*baseCE32s++; }

        if(length != baseLength) {
            add(c);
            return;
        }
        for(int32_t i = 0; i < length; ++i) {
            if(ce32s[i] != baseCE32s[i]) {
                add(c);
                break;
            }
        }
    } else if(tag == Collation::EXPANSION_TAG) {
        const int64_t *ces = data->ces + Collation::getExpansionIndex(ce32);
        int32_t length = Collation::getExpansionLength(ce32);
        if(length == 0) { length = (int32_t)*ces++; }

        const int64_t *baseCEs = baseData->ces + Collation::getExpansionIndex(baseCE32);
        int32_t baseLength = Collation::getExpansionLength(baseCE32);
        if(baseLength == 0) { baseLength = (int32_t)*baseCEs++; }

        if(length != baseLength) {
            add(c);
            return;
        }
        for(int32_t i = 0; i < length; ++i) {
            if(ces[i] != baseCEs[i]) {
                add(c);
                break;
            }
        }
    } else if(tag == Collation::HANGUL_TAG) {
        UChar jamos[3];
        int32_t length = Hangul::decompose(c, jamos);
        const int64_t *jamoCEs = data->jamoCEs;
        const int64_t *baseJamoCEs = baseData->jamoCEs;
        UChar l = jamos[0];
        UChar v = jamos[1];
        if(jamoCEs[l] != baseJamoCEs[l] ||
                jamoCEs[19 + v] != baseJamoCEs[19 + v] ||
                (length == 3 && jamoCEs[39 + jamos[2]] != baseJamoCEs[39 + jamos[2]])) {
            add(c);
        }
    } else if(ce32 != baseCE32) {
        add(c);
    }
}

void
TailoredSet::comparePrefixes(UChar32 c, const UChar *p, const UChar *q) {
    // Parallel iteration over prefixes of both tables.
    UCharsTrie::Iterator prefixes(p, 0, errorCode);
    UCharsTrie::Iterator basePrefixes(q, 0, errorCode);
    const UnicodeString *tp = NULL;  // Tailoring prefix.
    const UnicodeString *bp = NULL;  // Base prefix.
    // Use a string with a U+FFFF as the limit sentinel.
    // U+FFFF is untailorable and will not occur in prefixes.
    UnicodeString none((UChar)0xffff);
    while(tp != &none || bp != &none) {
        if(tp == NULL) {
            if(prefixes.next(errorCode)) {
                tp = &prefixes.getString();
            } else {
                tp = &none;
            }
        }
        if(bp == NULL) {
            if(basePrefixes.next(errorCode)) {
                bp = &basePrefixes.getString();
            } else {
                bp = &none;
            }
        }
        int32_t cmp = tp->compare(*bp);
        if(cmp < 0) {
            // tp occurs in the tailoring but not in the base.
            addPrefix(data, *tp, c, (uint32_t)prefixes.getValue());
            tp = NULL;
        } else if(cmp > 0) {
            // bp occurs in the base but not in the tailoring.
            addPrefix(baseData, *bp, c, (uint32_t)basePrefixes.getValue());
            bp = NULL;
        } else {
            prefix = tp;
            compare(c, (uint32_t)prefixes.getValue(), (uint32_t)basePrefixes.getValue());
            prefix = NULL;
            tp = NULL;
            bp = NULL;
        }
    }
}

void
TailoredSet::compareContractions(UChar32 c, const UChar *p, const UChar *q) {
    // Parallel iteration over suffixes of both tables.
    UCharsTrie::Iterator suffixes(p, 0, errorCode);
    UCharsTrie::Iterator baseSuffixes(q, 0, errorCode);
    const UnicodeString *ts = NULL;  // Tailoring suffix.
    const UnicodeString *bs = NULL;  // Base suffix.
    // Use a string with two U+FFFF as the limit sentinel.
    // U+FFFF is untailorable and will not occur in contractions except maybe
    // as a single suffix character for a root-collator boundary contraction.
    UnicodeString none((UChar)0xffff);
    none.append((UChar)0xffff);
    while(ts != &none || bs != &none) {
        if(ts == NULL) {
            if(suffixes.next(errorCode)) {
                ts = &suffixes.getString();
            } else {
                ts = &none;
            }
        }
        if(bs == NULL) {
            if(baseSuffixes.next(errorCode)) {
                bs = &baseSuffixes.getString();
            } else {
                bs = &none;
            }
        }
        int32_t cmp = ts->compare(*bs);
        if(cmp < 0) {
            // ts occurs in the tailoring but not in the base.
            addSuffix(c, *ts);
            ts = NULL;
        } else if(cmp > 0) {
            // bs occurs in the base but not in the tailoring.
            addSuffix(c, *bs);
            bs = NULL;
        } else {
            suffix = ts;
            compare(c, (uint32_t)suffixes.getValue(), (uint32_t)baseSuffixes.getValue());
            suffix = NULL;
            ts = NULL;
            bs = NULL;
        }
    }
}

void
TailoredSet::addPrefixes(const CollationData *d, UChar32 c, const UChar *p) {
    UCharsTrie::Iterator prefixes(p, 0, errorCode);
    while(prefixes.next(errorCode)) {
        addPrefix(d, prefixes.getString(), c, (uint32_t)prefixes.getValue());
    }
}

void
TailoredSet::addPrefix(const CollationData *d, const UnicodeString &pfx, UChar32 c, uint32_t ce32) {
    ce32 = d->getFinalCE32(ce32);
    if(Collation::isContractionCE32(ce32)) {
        const UChar *p = d->contexts + Collation::getContractionIndex(ce32);
        prefix = &pfx;
        addContractions(c, p + 2);
        prefix = NULL;
    }
    tailored->add(UnicodeString(pfx).append(c));
}

void
TailoredSet::addContractions(UChar32 c, const UChar *p) {
    UCharsTrie::Iterator suffixes(p, 0, errorCode);
    while(suffixes.next(errorCode)) {
        addSuffix(c, suffixes.getString());
    }
}

void
TailoredSet::addSuffix(UChar32 c, const UnicodeString &sfx) {
    UnicodeString s;
    if(prefix != NULL) {
        s.append(*prefix);
    }
    tailored->add(s.append(c).append(sfx));
}

void
TailoredSet::add(UChar32 c) {
    if(prefix == NULL && suffix == NULL) {
        tailored->add(c);
    } else {
        UnicodeString s;
        if(prefix != NULL) {
            s.append(*prefix);
        }
        s.append(c);
        if(suffix != NULL) {
            s.append(*suffix);
        }
        tailored->add(s);
    }
}

U_CDECL_BEGIN

static UBool U_CALLCONV
enumCnERange(const void *context, UChar32 start, UChar32 end, uint32_t ce32) {
    ContractionsAndExpansions *cne = (ContractionsAndExpansions *)context;
    if(cne->checkTailored == 0) {
        // There is no tailoring.
        // No need to collect nor check the tailored set.
    } else if(cne->checkTailored < 0) {
        // Collect the set of code points with mappings in the tailoring data.
        if(ce32 == Collation::MIN_SPECIAL_CE32) {
            return TRUE;  // fallback to base, not tailored
        } else {
            cne->tailored.add(start, end);
        }
        // checkTailored > 0: Exclude tailored ranges from the base data enumeration.
    } else if(start == end) {
        if(cne->tailored.contains(start)) {
            return TRUE;
        }
    } else if(cne->tailored.containsSome(start, end)) {
        cne->ranges.set(start, end).removeAll(cne->tailored);
        int32_t count = cne->ranges.getRangeCount();
        for(int32_t i = 0; i < count; ++i) {
            cne->handleCE32(cne->ranges.getRangeStart(i), cne->ranges.getRangeEnd(i), ce32);
        }
        return U_SUCCESS(cne->errorCode);
    }
    cne->handleCE32(start, end, ce32);
    return U_SUCCESS(cne->errorCode);
}

U_CDECL_END

void
ContractionsAndExpansions::forData(const CollationData *d, UErrorCode &ec) {
    if(U_FAILURE(ec)) { return; }
    errorCode = ec;  // Preserve info & warning codes.
    // Add all from the data, can be tailoring or base.
    if(d->base != NULL) {
        checkTailored = -1;
    }
    data = d;
    utrie2_enum(data->trie, NULL, enumCnERange, this);
    if(d->base == NULL || U_FAILURE(errorCode)) {
        ec = errorCode;
        return;
    }
    // Add all from the base data but only for un-tailored code points.
    tailored.freeze();
    checkTailored = 1;
    tailoring = d;
    data = d->base;
    utrie2_enum(data->trie, NULL, enumCnERange, this);
    ec = errorCode;
}

void
ContractionsAndExpansions::handleCE32(UChar32 start, UChar32 end, uint32_t ce32) {
    for(;;) {
        if(ce32 <= Collation::MIN_SPECIAL_CE32) {
            // !isSpecialCE32(), or fallback to the base
            return;
        }
        // Loop while ce32 is special.
        int32_t tag = Collation::getSpecialCE32Tag(ce32);
        if(tag <= Collation::EXPANSION_TAG || tag == Collation::HANGUL_TAG) {
            // Optimization: If we have a prefix,
            // then the relevant strings have been added already.
            if(prefix == NULL) {
                addExpansions(start, end);
            }
            return;
        } else if(tag == Collation::PREFIX_TAG) {
            handlePrefixes(start, end, ce32);
            return;
        } else if(tag == Collation::CONTRACTION_TAG) {
            handleContractions(start, end, ce32);
            return;
        } else if(tag == Collation::DIGIT_TAG) {
            // Fetch the non-numeric-collation CE32 and continue.
            ce32 = data->ce32s[Collation::getDigitIndex(ce32)];
        } else if(tag == Collation::RESERVED_TAG_11 || tag == Collation::LEAD_SURROGATE_TAG) {
            if(U_SUCCESS(errorCode)) { errorCode = U_INTERNAL_PROGRAM_ERROR; }
            return;
        } else if(tag == Collation::IMPLICIT_TAG && (ce32 & 1) == 0) {
            U_ASSERT(start == 0 && end == 0);
            // Fetch the normal ce32 for U+0000 and continue.
            ce32 = data->ce32s[0];
        } else {
            return;
        }
    }
}

void
ContractionsAndExpansions::handlePrefixes(
        UChar32 start, UChar32 end, uint32_t ce32) {
    const UChar *p = data->contexts + Collation::getPrefixIndex(ce32);
    ce32 = ((uint32_t)p[0] << 16) | p[1];  // Default if no prefix match.
    handleCE32(start, end, ce32);
    if(!addPrefixes) { return; }
    UCharsTrie::Iterator prefixes(p + 2, 0, errorCode);
    while(prefixes.next(errorCode)) {
        prefix = &prefixes.getString();
        // Prefix/pre-context mappings are special kinds of contractions
        // that always yield expansions.
        addStrings(start, end, contractions);
        addStrings(start, end, expansions);
        handleCE32(start, end, (uint32_t)prefixes.getValue());
    }
    prefix = NULL;
}

void
ContractionsAndExpansions::handleContractions(
        UChar32 start, UChar32 end, uint32_t ce32) {
    const UChar *p = data->contexts + Collation::getContractionIndex(ce32);
    ce32 = ((uint32_t)p[0] << 16) | p[1];  // Default if no suffix match.
    // Ignore the default mapping if it falls back to another set of contractions:
    // In that case, we are underneath a prefix, and the empty prefix
    // maps to the same contractions.
    if(Collation::isContractionCE32(ce32)) {
        U_ASSERT(prefix != NULL);
    } else {
        handleCE32(start, end, ce32);
    }
    UCharsTrie::Iterator suffixes(p + 2, 0, errorCode);
    while(suffixes.next(errorCode)) {
        suffix = &suffixes.getString();
        addStrings(start, end, contractions);
        if(prefix != NULL) {
            addStrings(start, end, expansions);
        }
        handleCE32(start, end, (uint32_t)suffixes.getValue());
    }
    suffix = NULL;
}

void
ContractionsAndExpansions::addExpansions(UChar32 start, UChar32 end) {
    if(prefix == NULL && suffix == NULL) {
        if(expansions != NULL) {
            // TODO: verify that UnicodeSet takes a fastpath if start==end
            expansions->add(start, end);
        }
    } else {
        addStrings(start, end, expansions);
    }
}

void
ContractionsAndExpansions::addStrings(UChar32 start, UChar32 end, UnicodeSet *set) {
    if(set == NULL) { return; }
    UnicodeString s;
    int32_t prefixLength;
    if(prefix != NULL) {
        s = *prefix;
        prefixLength = prefix->length();
    } else {
        prefixLength = 0;
    }
    do {
        s.append(start);
        if(suffix != NULL) {
            s.append(*suffix);
        }
        expansions->add(s);
        s.truncate(prefixLength);
    } while(++start <= end);
}

U_NAMESPACE_END

#endif  // !UCONFIG_NO_COLLATION