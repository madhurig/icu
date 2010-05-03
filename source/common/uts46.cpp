/*
*******************************************************************************
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  uts46.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010mar09
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/idna.h"
#include "unicode/normalizer2.h"
#include "punycode.h"

U_NAMESPACE_BEGIN

// UTS46 class declaration ------------------------------------------------- ***

class UTS46 : public IDNA {
public:
    UTS46(uint32_t options, UErrorCode &errorCode);
    virtual ~UTS46();

    virtual UnicodeString &
    labelToASCII(const UnicodeString &label, UnicodeString &dest,
                 IDNAInfo &info, UErrorCode &errorCode) const;

    virtual UnicodeString &
    labelToUnicode(const UnicodeString &label, UnicodeString &dest,
                   IDNAInfo &info, UErrorCode &errorCode) const;

    virtual UnicodeString &
    nameToASCII(const UnicodeString &name, UnicodeString &dest,
                IDNAInfo &info, UErrorCode &errorCode) const;

    virtual UnicodeString &
    nameToUnicode(const UnicodeString &name, UnicodeString &dest,
                  IDNAInfo &info, UErrorCode &errorCode) const;

    static UClassID U_EXPORT2 getStaticClassID();
    virtual UClassID getDynamicClassID() const;

private:
    UnicodeString &
    process(const UnicodeString &src,
            UBool isLabel, UBool toASCII,
            UnicodeString &dest,
            IDNAInfo &info, UErrorCode &errorCode) const;
    UnicodeString &
    processUnicode(const UnicodeString &src,
                   int32_t labelStart, int32_t mappingStart,
                   UBool isLabel, UBool toASCII,
                   UnicodeString &dest,
                   IDNAInfo &info, UErrorCode &errorCode) const;

    // returns delta for how much the label length changes
    int32_t
    processLabel(UnicodeString &dest,
                 int32_t labelStart, int32_t labelLength,
                 UBool toASCII,
                 IDNAInfo &info, UErrorCode &errorCode) const;

    UBool
    isLabelOkBiDi(const UChar *label, int32_t labelLength) const;

    UBool
    isLabelOkContextJ(const UChar *label, int32_t labelLength) const;

    const Normalizer2 &uts46Norm2;  // uts46.nrm
    const Normalizer2 &nfcNorm2;
    uint32_t options;
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UTS46)

IDNA *
IDNA::createUTS46Instance(uint32_t options, UErrorCode &errorCode) {
    if(U_SUCCESS(errorCode)) {
        return new UTS46(options, errorCode);
    } else {
        return NULL;
    }
}

UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(IDNA)

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(IDNAInfo)

// UTS46 implementation ---------------------------------------------------- ***

UTS46::UTS46(uint32_t opt, UErrorCode &errorCode)
        : uts46Norm2(*Normalizer2::getInstance(NULL, "uts46", UNORM2_COMPOSE, errorCode)),
          nfcNorm2(*Normalizer2::getInstance(NULL, "nfc", UNORM2_COMPOSE, errorCode)),
          options(opt) {}

UTS46::~UTS46() {}

UnicodeString &
UTS46::labelToASCII(const UnicodeString &label, UnicodeString &dest,
                    IDNAInfo &info, UErrorCode &errorCode) const {
    process(label, TRUE, TRUE, dest, info, errorCode);
    if(info.hasErrors()) {
        dest.setToBogus();
    }
    return dest;
}

UnicodeString &
UTS46::labelToUnicode(const UnicodeString &label, UnicodeString &dest,
                      IDNAInfo &info, UErrorCode &errorCode) const {
    return process(label, TRUE, FALSE, dest, info, errorCode);
}

UnicodeString &
UTS46::nameToASCII(const UnicodeString &name, UnicodeString &dest,
                   IDNAInfo &info, UErrorCode &errorCode) const {
    process(name, FALSE, TRUE, dest, info, errorCode);
    if(dest.length()>=254 && (dest.length()>254 || dest[253]!=0x2e)) {
        info.errors|=UIDNA_ERROR_DOMAIN_NAME_TOO_LONG;
    }
    if(info.hasErrors()) {
        dest.setToBogus();
    }
    return dest;
}

UnicodeString &
UTS46::nameToUnicode(const UnicodeString &name, UnicodeString &dest,
                     IDNAInfo &info, UErrorCode &errorCode) const {
    return process(name, FALSE, FALSE, dest, info, errorCode);
}

// UTS #46 data for ASCII characters.
// The normalizer (using uts46.nrm) maps uppercase ASCII letters to lowercase
// and passes through all other ASCII characters.
// If UIDNA_USE_STD3_RULES is set, then non-LDH characters are disallowed
// using this data.
// The ASCII fastpath also uses this data.
// Values: -1=disallowed  0==valid  1==mapped (lowercase)
static const int8_t asciiData[128]={
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    // 002D..002E; valid  #  HYPHEN-MINUS..FULL STOP
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0, -1,
    // 0030..0039; valid  #  DIGIT ZERO..DIGIT NINE
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1, -1,
    // 0041..005A; mapped  #  LATIN CAPITAL LETTER A..LATIN CAPITAL LETTER Z
    -1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, -1, -1, -1, -1, -1,
    // 0061..007A; valid  #  LATIN SMALL LETTER A..LATIN SMALL LETTER Z
    -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1
};

UnicodeString &
UTS46::process(const UnicodeString &src,
               UBool isLabel, UBool toASCII,
               UnicodeString &dest,
               IDNAInfo &info, UErrorCode &errorCode) const {
    // uts46Norm2.normalize() would do all of this error checking and setup,
    // but with the ASCII fastpath we do not always call it, and do not
    // call it first.
    if(U_FAILURE(errorCode)) {
        dest.setToBogus();
        return dest;
    }
    const UChar *srcArray=src.getBuffer();
    if(&dest==&src || srcArray==NULL) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        dest.setToBogus();
        return dest;
    }
    // Arguments are fine, reset output values.
    dest.remove();
    info.reset();
    int32_t srcLength=src.length();
    if(srcLength==0) {
        info.errors|=UIDNA_ERROR_EMPTY_LABEL;
        return dest;
    }
    UChar *destArray=dest.getBuffer(srcLength);
    if(destArray==NULL) {
        errorCode=U_MEMORY_ALLOCATION_ERROR;
        return dest;
    }
    // ASCII fastpath
    UBool disallowNonLDHDot=(options&UIDNA_USE_STD3_RULES)!=0;
    int32_t labelStart=0;
    int32_t i;
    for(i=0;; ++i) {
        if(i==srcLength) {
            if(toASCII && (i-labelStart)>63) {
                info.errors|=UIDNA_ERROR_LABEL_TOO_LONG;
            }
            dest.releaseBuffer(i);
            return dest;
        }
        UChar c=srcArray[i];
        if(c>0x7f) {
            break;
        }
        int cData=asciiData[c];
        if(cData>0) {
            destArray[i]=c+0x20;  // Lowercase an uppercase ASCII letter.
        } else if(cData<0 && disallowNonLDHDot) {
            break;  // Replacing with U+FFFD can be complicated for toASCII.
        } else {
            destArray[i]=c;
            if(c==0x2d) {  // hyphen
                if(i==(labelStart+3) && srcArray[i-1]==0x2d) {
                    // "??--..." is Punycode or forbidden.
                    break;
                }
                if(i==labelStart) {
                    // label starts with "-"
                    info.errors|=UIDNA_ERROR_LEADING_HYPHEN;
                }
                if((i+1)==srcLength || srcArray[i+1]==0x2e) {
                    // label ends with "-"
                    info.errors|=UIDNA_ERROR_TRAILING_HYPHEN;
                }
            } else if(c==0x2e) {  // dot
                if(isLabel) {
                    break;  // Replacing with U+FFFD can be complicated for toASCII.
                }
                // Permit an empty label at the end but not elsewhere.
                if(i==labelStart && i<(srcLength-1)) {
                    info.errors|=UIDNA_ERROR_EMPTY_LABEL;
                } else if(toASCII && (i-labelStart)>63) {
                    info.errors|=UIDNA_ERROR_LABEL_TOO_LONG;
                }
                labelStart=i+1;
            }
        }
    }
    dest.releaseBuffer(i);
    return processUnicode(src, labelStart, i, isLabel, toASCII, dest, info, errorCode);
}

UnicodeString &
UTS46::processUnicode(const UnicodeString &src,
                      int32_t labelStart, int32_t mappingStart,
                      UBool isLabel, UBool toASCII,
                      UnicodeString &dest,
                      IDNAInfo &info, UErrorCode &errorCode) const {
    if(mappingStart==0) {
        uts46Norm2.normalize(src, dest, errorCode);
    } else {
        uts46Norm2.normalizeSecondAndAppend(dest, src.tempSubString(mappingStart), errorCode);
    }
    if(U_FAILURE(errorCode)) {
        return dest;
    }
    if(isLabel) {
        processLabel(dest, 0, dest.length(), toASCII, info, errorCode);
    } else {
        const UChar *destArray=dest.getBuffer();
        int32_t destLength=dest.length();
        int32_t labelLimit=labelStart, delta;
        while(labelLimit<destLength) {
            if(destArray[labelLimit]==0x2e) {
                delta=processLabel(dest, labelStart, labelLimit-labelStart,
                                   toASCII, info, errorCode);
                if(U_FAILURE(errorCode)) {
                    return dest;
                }
                destArray=dest.getBuffer();
                destLength+=delta;
                labelStart=labelLimit+=delta+1;
            } else {
                ++labelLimit;
            }
        }
        // Permit an empty label at the end (0<labelStart==labelLimit==destLength is ok)
        // but not an empty label elsewhere nor a completely empty domain name.
        // processLabel() sets UIDNA_ERROR_EMPTY_LABEL when labelLength==0.
        if(0==labelStart || labelStart<labelLimit) {
            processLabel(dest, labelStart, labelLimit-labelStart,
                         toASCII, info, errorCode);
        }
    }
    return dest;
}

// Replace the label in dest with the label string, if the label was modified.
// If &label==&dest then the label was modified in-place and labelLength
// is the new length, different from label.length().
// If &label!=&dest then labelLength==label.length().
// Returns the delta of the new vs. old label length.
static int32_t
replaceLabel(UnicodeString &dest, int32_t destLabelStart, int32_t destLabelLength,
             const UnicodeString &label, int32_t labelLength) {
    if(&label!=&dest) {
        dest.replace(destLabelStart, destLabelLength, label);
    }
    return labelLength-destLabelLength;
}

int32_t
UTS46::processLabel(UnicodeString &dest,
                    int32_t labelStart, int32_t labelLength,
                    UBool toASCII,
                    IDNAInfo &info, UErrorCode &errorCode) const {
    UnicodeString fromPunycode;
    UnicodeString *labelString;
    const UChar *label=dest.getBuffer()+labelStart;
    int32_t destLabelStart=labelStart;
    int32_t destLabelLength=labelLength;
    UBool wasPunycode;
    if(labelLength>=4 && label[0]==0x78 && label[1]==0x6e && label[2]==0x2d && label[3]==0x2d) {
        // Label starts with "xn--", try to un-Punycode it.
        wasPunycode=TRUE;
        UnicodeString unicode;
        UChar *unicodeBuffer=unicode.getBuffer(-1);  // capacity==-1: most labels should fit
        if(unicodeBuffer==NULL) {
            // Should never occur if we used capacity==-1 which uses the internal buffer.
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        UErrorCode punycodeErrorCode=U_ZERO_ERROR;
        int32_t unicodeLength=u_strFromPunycode(label+4, labelLength-4,
                                                unicodeBuffer, unicode.getCapacity(),
                                                NULL, &punycodeErrorCode);
        if(punycodeErrorCode==U_BUFFER_OVERFLOW_ERROR) {
            unicode.releaseBuffer(0);
            unicodeBuffer=unicode.getBuffer(unicodeLength);
            if(unicodeBuffer==NULL) {
                errorCode=U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }
            punycodeErrorCode=U_ZERO_ERROR;
            unicodeLength=u_strFromPunycode(label+4, labelLength-4,
                                            unicodeBuffer, unicode.getCapacity(),
                                            NULL, &punycodeErrorCode);
        }
        unicode.releaseBuffer(unicodeLength);
        if(U_FAILURE(punycodeErrorCode)) {
            info.errors|=UIDNA_ERROR_PUNYCODE;
            // Append U+FFFD if the label has only LDH characters.
            // If UIDNA_USE_STD3_RULES, also replace disallowed ASCII characters with U+FFFD.
            UBool disallowNonLDHDot=(options&UIDNA_USE_STD3_RULES)!=0;
            UBool onlyLDH=TRUE;
            // Ok to cast away const because we own the UnicodeString.
            UChar *s=(UChar *)label+4;  // After the initial "xn--".
            const UChar *limit=label+labelLength;
            do {
                UChar c=*s;
                if(c<=0x7f) {
                    if(c==0x2e) {
                        info.errors|=UIDNA_ERROR_LABEL_HAS_DOT;
                        onlyLDH=FALSE;
                        *s=0xfffd;
                    } else if(asciiData[c]<0) {
                        onlyLDH=FALSE;
                        if(disallowNonLDHDot) {
                            *s=0xfffd;
                        }
                    }
                } else {
                    onlyLDH=FALSE;
                }
            } while(++s<limit);
            if(onlyLDH) {
                dest.insert(labelStart+labelLength, (UChar)0xfffd);
                return 1;
            } else {
                return 0;
            }
        }
        // Check for NFC, and for characters that are not
        // valid or deviation characters according to the normalizer.
        // If there is something wrong, then the string will change.
        // Note that the normalizer passes through non-LDH ASCII and deviation characters.
        // Deviation characters are ok in Punycode even in transitional processing.
        // In the code further below, if we find non-LDH ASCII and we have UIDNA_USE_STD3_RULES
        // then we will set UIDNA_ERROR_INVALID_ACE_LABEL there too.
        uts46Norm2.normalize(unicode, fromPunycode, errorCode);
        if(U_FAILURE(errorCode)) {
            return 0;
        }
        if(unicode!=fromPunycode) {
            info.errors|=UIDNA_ERROR_INVALID_ACE_LABEL;
        }
        labelString=&fromPunycode;
        label=fromPunycode.getBuffer();
        labelStart=0;
        labelLength=fromPunycode.length();
    } else {
        wasPunycode=FALSE;
        labelString=&dest;
    }
    // Validity check
    if(labelLength==0) {
        if(toASCII) {
            info.errors|=UIDNA_ERROR_EMPTY_LABEL;
        }
        return replaceLabel(dest, destLabelStart, destLabelLength, *labelString, labelLength);
    }
    // labelLength>0
    if(labelLength>=4 && label[2]==0x2d && label[3]==0x2d) {
        // label starts with "??--"
        info.errors|=UIDNA_ERROR_HYPHEN_3_4;
    }
    if(label[0]==0x2d) {
        // label starts with "-"
        info.errors|=UIDNA_ERROR_LEADING_HYPHEN;
    }
    if(label[labelLength-1]==0x2d) {
        // label ends with "-"
        info.errors|=UIDNA_ERROR_TRAILING_HYPHEN;
    }
    UChar32 c;
    int32_t cpLength=0;
    // "Unsafe" is ok because unpaired surrogates were mapped to U+FFFD.
    U16_NEXT_UNSAFE(label, cpLength, c);
    if((U_GET_GC_MASK(c)&U_GC_M_MASK)!=0) {
        info.errors|=UIDNA_ERROR_LEADING_COMBINING_MARK;
        labelString->replace(labelStart, cpLength, (UChar)0xfffd);
        label=labelString->getBuffer()+labelStart;
        labelLength+=1-cpLength;
    }
    // If the label was not a Punycode label, then it was the result of
    // mapping, normalization and label segmentation.
    // If the label was in Punycode, then we mapped it again above
    // and checked its validity.
    // Now we handle the STD3 restriction to LDH characters (if set)
    // and the deviation characters (transitional vs. nontransitional),
    // and we look for U+FFFD which indicates disallowed characters
    // in a non-Punycode label or U+FFFD itself in a Punycode label.
    // We also check for dots which can come from a Punycode label
    // or from the input to a single-label function.
    // Ok to cast away const because we own the UnicodeString.
    UChar *s=(UChar *)label;
    const UChar *limit=label+labelLength;
    UChar oredChars=0;
    // If we enforce STD3 rules, then ASCII characters other than LDH and dot are disallowed.
    UBool disallowNonLDHDot=(options&UIDNA_USE_STD3_RULES)!=0;
    UBool doMapDevChars=
        !wasPunycode &&  // Always pass through deviation characters from Punycode.
        (toASCII ? (options&UIDNA_NONTRANSITIONAL_TO_ASCII)==0 :
                   (options&UIDNA_NONTRANSITIONAL_TO_UNICODE)==0);
    UBool didMapDevChars=FALSE;
    do {
        UChar c=*s;
        if(c<=0x7f) {
            if(c==0x2e) {
                info.errors|=UIDNA_ERROR_LABEL_HAS_DOT;
                *s=0xfffd;
            } else if(disallowNonLDHDot && asciiData[c]<0) {
                info.errors|=UIDNA_ERROR_DISALLOWED;
                if(wasPunycode) {
                    info.errors|=UIDNA_ERROR_INVALID_ACE_LABEL;
                }
                *s=0xfffd;
            }
        } else {
            oredChars|=c;
            switch(c) {
            case 0xdf:
                info.hasDevChars=TRUE;
                if(doMapDevChars) {  // Map sharp s to ss.
                    didMapDevChars=TRUE;
                    *s++=0x73;  // Replace sharp s with first s.
                    // Insert second s and account for possible buffer reallocation.
                    int32_t labelStringIndex=(int32_t)(s-labelString->getBuffer());
                    labelString->insert(labelStringIndex, (UChar)0x73);
                    ++labelLength;
                    label=labelString->getBuffer();
                    s=(UChar *)label+labelStringIndex;
                    label+=labelStart;
                    limit=label+labelLength;
                }
                break;
            case 0x3c2:  // Map final sigma to nonfinal sigma.
                info.hasDevChars=TRUE;
                if(doMapDevChars) {
                    didMapDevChars=TRUE;
                    *s=0x3c3;
                }
                break;
            case 0x200c:  // Ignore/remove ZWNJ.
            case 0x200d:  // Ignore/remove ZWJ.
                info.hasDevChars=TRUE;
                if(doMapDevChars) {
                    didMapDevChars=TRUE;
                    // Removing a character should not cause the UnicodeString
                    // buffer to be reallocated, but we don't want to assume that.
                    int32_t labelStringIndex=(int32_t)(s-labelString->getBuffer());
                    labelString->remove(labelStringIndex, 1);
                    --labelLength;
                    label=labelString->getBuffer();
                    s=(UChar *)label+labelStringIndex;
                    label+=labelStart;
                    limit=label+labelLength;
                    continue;  // Skip the ++s at the end of the loop.
                }
                break;
            case 0xfffd:
                info.errors|=UIDNA_ERROR_DISALLOWED;
                break;
            }
        }
        ++s;
    } while(s<limit);
    UnicodeString normalized;
    if(didMapDevChars) {
        // Mapping deviation characters might have resulted in an un-NFC string.
        nfcNorm2.normalize(
            labelString==&dest ?
                dest.tempSubString(labelStart, labelLength) :
                *labelString,
            normalized, errorCode);
        if(U_FAILURE(errorCode)) {
            return replaceLabel(dest, destLabelStart, destLabelLength, *labelString, labelLength);
        }
        labelString=&normalized;
        label=normalized.getBuffer();
        labelStart=0;
        labelLength=normalized.length();
    }
    if( (options&UIDNA_CHECK_BIDI)!=0 && oredChars>=0x590 &&
        !isLabelOkBiDi(label, labelLength)
    ) {
        info.errors|=UIDNA_ERROR_BIDI;
    }
    if( (options&UIDNA_CHECK_CONTEXTJ)!=0 && (oredChars&0x200c)==0x200c &&
        !isLabelOkContextJ(label, labelLength)
    ) {
        info.errors|=UIDNA_ERROR_CONTEXTJ;
    }
    if( toASCII && 
        (wasPunycode ?
            // If wasPunycode, we want to re-Punycode the label only if it was
            // modified. Checking info.errors is simpler and safer than tracking
            // "was modified" precisely, although suboptimal for domain names with
            // errors in other labels or errors that did not modify the label
            // (e.g., leading/trailing hyphen).
            (didMapDevChars || info.errors!=0):
            // If the label was not originally Punycode, then check for non-ASCII characters.
            oredChars>=0x80)
    ) {
        UnicodeString punycode;
        UChar *buffer=punycode.getBuffer(63);  // 63==maximum DNS label length
        if(buffer==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
        } else {
            buffer[0]=0x78;  // Write "xn--".
            buffer[1]=0x6e;
            buffer[2]=0x2d;
            buffer[3]=0x2d;
            int32_t punycodeLength=u_strToPunycode(label, labelLength,
                                                  buffer+4, punycode.getCapacity()-4,
                                                  NULL, &errorCode);
            if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
                errorCode=U_ZERO_ERROR;
                punycode.releaseBuffer(4);
                buffer=punycode.getBuffer(punycodeLength);
                if(buffer==NULL) {
                    errorCode=U_MEMORY_ALLOCATION_ERROR;
                } else {
                    punycodeLength=u_strToPunycode(label, labelLength,
                                                   buffer+4, punycode.getCapacity()-4,
                                                   NULL, &errorCode);
                }
            }
            punycodeLength+=4;
            punycode.releaseBuffer(punycodeLength);
            if(U_SUCCESS(errorCode)) {
                if(punycodeLength>63) {
                    info.errors|=UIDNA_ERROR_LABEL_TOO_LONG;
                }
                return replaceLabel(dest, destLabelStart, destLabelLength,
                                    punycode, punycodeLength);
            }
        }
    }
    return replaceLabel(dest, destLabelStart, destLabelLength, *labelString, labelLength);
}

#define L_MASK U_MASK(U_LEFT_TO_RIGHT)
#define R_AL_MASK U_MASK(U_RIGHT_TO_LEFT)|U_MASK(U_RIGHT_TO_LEFT_ARABIC)
#define L_R_AL_MASK L_MASK|R_AL_MASK

#define EN_AN_MASK U_MASK(U_EUROPEAN_NUMBER)|U_MASK(U_ARABIC_NUMBER)
#define R_AL_EN_AN_MASK R_AL_MASK|EN_AN_MASK
#define L_EN_MASK L_MASK|U_MASK(U_EUROPEAN_NUMBER)

#define ES_CS_ET_ON_BN_NSM_MASK \
    U_MASK(U_EUROPEAN_NUMBER_SEPARATOR)| \
    U_MASK(U_COMMON_NUMBER_SEPARATOR)| \
    U_MASK(U_EUROPEAN_NUMBER_TERMINATOR)| \
    U_MASK(U_OTHER_NEUTRAL)| \
    U_MASK(U_BOUNDARY_NEUTRAL)| \
    U_MASK(U_DIR_NON_SPACING_MARK)
#define L_EN_ES_CS_ET_ON_BN_NSM_MASK L_EN_MASK|ES_CS_ET_ON_BN_NSM_MASK
#define R_AL_AN_EN_ES_CS_ET_ON_BN_NSM_MASK R_AL_MASK|EN_AN_MASK|ES_CS_ET_ON_BN_NSM_MASK

UBool
UTS46::isLabelOkBiDi(const UChar *label, int32_t labelLength) const {
    // IDNA2008 BiDi rule
    // Get the directionality of the first character.
    UChar32 c;
    int32_t i=0;
    U16_NEXT_UNSAFE(label, i, c);
    uint32_t firstMask=U_MASK(u_charDirection(c));
    // 1. The first character must be a character with BIDI property L, R
    // or AL.  If it has the R or AL property, it is an RTL label; if it
    // has the L property, it is an LTR label.
    if((firstMask&~L_R_AL_MASK)!=0) {
        return FALSE;
    }
    // Get the directionality of the last non-NSM character.
    uint32_t lastMask;
    for(;;) {
        if(i>=labelLength) {
            lastMask=firstMask;
            break;
        }
        U16_PREV_UNSAFE(label, labelLength, c);
        UCharDirection dir=u_charDirection(c);
        if(dir!=U_DIR_NON_SPACING_MARK) {
            lastMask=U_MASK(dir);
            break;
        }
    }
    // 3. In an RTL label, the end of the label must be a character with
    // BIDI property R, AL, EN or AN, followed by zero or more
    // characters with BIDI property NSM.
    // 6. In an LTR label, the end of the label must be a character with
    // BIDI property L or EN, followed by zero or more characters with
    // BIDI property NSM.
    if( (firstMask&L_MASK)!=0 ?
            (lastMask&~L_EN_MASK)!=0 :
            (lastMask&~R_AL_EN_AN_MASK)!=0
    ) {
        return FALSE;
    }
    // Get the directionalities of the intervening characters.
    uint32_t mask=0;
    while(i<labelLength) {
        U16_NEXT_UNSAFE(label, i, c);
        mask|=U_MASK(u_charDirection(c));
    }
    if(firstMask&L_MASK) {
        // 5. In an LTR label, only characters with the BIDI properties L, EN,
        // ES, CS.  ET, ON, BN and NSM are allowed.
        if((mask&~L_EN_ES_CS_ET_ON_BN_NSM_MASK)!=0) {
            return FALSE;
        }
    } else {
        // 2. In an RTL label, only characters with the BIDI properties R, AL,
        // AN, EN, ES, CS, ET, ON, BN and NSM are allowed.
        if((mask&~R_AL_AN_EN_ES_CS_ET_ON_BN_NSM_MASK)!=0) {
            return FALSE;
        }
        // 4. In an RTL label, if an EN is present, no AN may be present, and
        // vice versa.
        if((mask&EN_AN_MASK)==EN_AN_MASK) {
            return FALSE;
        }
    }
    return TRUE;
}

UBool
UTS46::isLabelOkContextJ(const UChar *label, int32_t labelLength) const {
    // [IDNA2008-Tables]
    // 200C..200D  ; CONTEXTJ    # ZERO WIDTH NON-JOINER..ZERO WIDTH JOINER
    for(int32_t i=0; i<labelLength; ++i) {
        if(label[i]==0x200c) {
            // Appendix A.1. ZERO WIDTH NON-JOINER
            // Rule Set:
            //  False;
            //  If Canonical_Combining_Class(Before(cp)) .eq.  Virama Then True;
            //  If RegExpMatch((Joining_Type:{L,D})(Joining_Type:T)*\u200C
            //     (Joining_Type:T)*(Joining_Type:{R,D})) Then True;
            if(i==0) {
                return FALSE;
            }
            UChar32 c;
            int32_t j=i;
            U16_PREV_UNSAFE(label, j, c);
            if(u_getCombiningClass(c)==9) {
                continue;
            }
            // check precontext (Joining_Type:{L,D})(Joining_Type:T)*
            for(;;) {
                UJoiningType type=(UJoiningType)u_getIntPropertyValue(c, UCHAR_JOINING_TYPE);
                if(type==U_JT_TRANSPARENT) {
                    if(j==0) {
                        return FALSE;
                    }
                    U16_PREV_UNSAFE(label, j, c);
                } else if(type==U_JT_LEFT_JOINING || type==U_JT_DUAL_JOINING) {
                    break;  // precontext fulfilled
                } else {
                    return FALSE;
                }
            }
            // check postcontext (Joining_Type:T)*(Joining_Type:{R,D})
            for(j=i+1;;) {
                if(j==labelLength) {
                    return FALSE;
                }
                U16_NEXT_UNSAFE(label, i, c);
                UJoiningType type=(UJoiningType)u_getIntPropertyValue(c, UCHAR_JOINING_TYPE);
                if(type==U_JT_TRANSPARENT) {
                    // just skip this character
                } else if(type==U_JT_RIGHT_JOINING || type==U_JT_DUAL_JOINING) {
                    break;  // postcontext fulfilled
                } else {
                    return FALSE;
                }
            }
        } else if(label[i]==0x200d) {
            // Appendix A.2. ZERO WIDTH JOINER (U+200D)
            // Rule Set:
            //  False;
            //  If Canonical_Combining_Class(Before(cp)) .eq.  Virama Then True;
            if(i==0) {
                return FALSE;
            }
            UChar32 c;
            int32_t j=i;
            U16_PREV_UNSAFE(label, j, c);
            if(u_getCombiningClass(c)!=9) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

U_NAMESPACE_END

#endif  // UCONFIG_NO_IDNA
