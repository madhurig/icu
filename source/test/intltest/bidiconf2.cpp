/*
*******************************************************************************
*
*   Copyright (C) 2009-2013, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  bidiconf2.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2013july1
*   created by: Matitiahu Allouche
*   adapted from bidiconf.cpp originally written by Markus W. Scherer
*
*   BiDi conformance test, using the Unicode BidiCharacterTest.txt file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unicode/utypes.h"
#include "unicode/ubidi.h"
#include "unicode/errorcode.h"
#include "unicode/localpointer.h"
#include "unicode/putil.h"
#include "unicode/unistr.h"
#include "intltest.h"
#include "uparse.h"

class BiDiConformanceTest2 : public IntlTest {
public:
    BiDiConformanceTest2() :
        directionBits(0), lineNumber(0), levelsCount(0), orderingCount(0),
        errorCount(0) {}

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);

    void TestBidiTest();
private:
    char *getUnidataPath(char path[]);

    UBool parseLevels(const char *&start);
    UBool parseOrdering(const char *start);
    UBool parseInputStringFromBiDiClasses(const char *&start);

    UBool checkLevels(const UBiDiLevel actualLevels[], int32_t actualCount,
                      const char *paraLevelName);
    UBool checkOrdering(UBiDi *ubidi, const char *paraLevelName);

    void printErrorLine(const char *paraLevelName);

    char line[10000];
    UBiDiLevel levels[1000];
    uint32_t directionBits;
    int32_t ordering[1000];
    int32_t lineNumber;
    int32_t levelsCount;
    int32_t orderingCount;
    int32_t errorCount;
    UnicodeString inputString;
};

extern IntlTest *createBiDiConformanceTest2() {
    return new BiDiConformanceTest2();
}

void BiDiConformanceTest2::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if(exec) {
        logln("TestSuite BiDiConformanceTest2: ");
    }
    switch (index) {
        TESTCASE(0, TestBidiTest);
        default:
            name="";
            break; // needed to end the loop
    }
}

// TODO: Move to a common place (IntlTest?) to avoid duplication with UnicodeTest (ucdtest.cpp).
char *BiDiConformanceTest2::getUnidataPath(char path[]) {
    IcuTestErrorCode errorCode(*this, "getUnidataPath");
    const int kUnicodeDataTxtLength=15;  // strlen("UnicodeData.txt")

    // Look inside ICU_DATA first.
    strcpy(path, pathToDataDirectory());
    strcat(path, "unidata" U_FILE_SEP_STRING "UnicodeData.txt");
    FILE *f=fopen(path, "r");
    if(f!=NULL) {
        fclose(f);
        *(strchr(path, 0)-kUnicodeDataTxtLength)=0;  // Remove the basename.
        return path;
    }

    // As a fallback, try to guess where the source data was located
    // at the time ICU was built, and look there.
#   ifdef U_TOPSRCDIR
        strcpy(path, U_TOPSRCDIR  U_FILE_SEP_STRING "data");
#   else
        strcpy(path, loadTestData(errorCode));
        strcat(path, U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".."
                     U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".."
                     U_FILE_SEP_STRING "data");
#   endif
    strcat(path, U_FILE_SEP_STRING);
    strcat(path, "unidata" U_FILE_SEP_STRING "UnicodeData.txt");
    f=fopen(path, "r");
    if(f!=NULL) {
        fclose(f);
        *(strchr(path, 0)-kUnicodeDataTxtLength)=0;  // Remove the basename.
        return path;
    }
    return NULL;
}

U_DEFINE_LOCAL_OPEN_POINTER(LocalStdioFilePointer, FILE, fclose);

UBool BiDiConformanceTest2::parseLevels(const char *&start) {
    directionBits=0;
    levelsCount=0;
    while(*start!=0 && *(start=u_skipWhitespace(start))!=0 && *start!=';') {
        if(*start=='x') {
            levels[levelsCount++]=UBIDI_DEFAULT_LTR;
            ++start;
        } else {
            char *end;
            uint32_t value=(uint32_t)strtoul(start, &end, 10);
            if(end<=start || (!U_IS_INV_WHITESPACE(*end) && *end!=0 && *end!=';') || value>(UBIDI_MAX_EXPLICIT_LEVEL+1)) {
                errln("\nError on line %d: Levels parse error at %s", (int)lineNumber, start);
                return FALSE;
            }
            levels[levelsCount++]=(UBiDiLevel)value;
            directionBits|=(1<<(value&1));
            start=end;
        }
    }
    return TRUE;
}

UBool BiDiConformanceTest2::parseOrdering(const char *start) {
    orderingCount=0;
    while(*start!=0 && *(start=u_skipWhitespace(start))!=0 && *start!=';') {
        char *end;
        uint32_t value=(uint32_t)strtoul(start, &end, 10);
        if(end<=start || (!U_IS_INV_WHITESPACE(*end) && *end!=0 && *end!=';') || value>=1000) {
            errln("\nError on line %d: Reorder parse error at %s", (int)lineNumber, start);
            return FALSE;
        }
        ordering[orderingCount++]=(int32_t)value;
        start=end;
    }
    return TRUE;
}

static const UChar charFromBiDiClass[U_CHAR_DIRECTION_COUNT]={
    0x006c,   // 'l' for L
    0x05d0,   // Hebrew Letter Alef for R
    0x0033,   // '3' for EN
    0x002d,   // '-' for ES
    0x0025,   // '%' for ET
    0x0669,   // Arabic-Indic '9' for AN
    0x002c,   // ',' for CS
    0x000d,   // CR  for B
    0x0009,   // Tab for S
    0x0020,   // ' ' for WS
    0x003d,   // '=' for ON
    0x202a,   // LRE
    0x202d,   // LRO
    0x0630,   // Arabic Letter Thal for AL
    0x202b,   // RLE
    0x202e,   // RLO
    0x202c,   // PDF
    0x05b9,   // Hebrew Point Holam for NSM
    0x00ad,   // Soft Hyphen for BN
    0x2068,   // FSI
    0x2066,   // LRI
    0x2067,   // RLI
    0x2069    // PDI
};

U_CDECL_BEGIN

U_CDECL_END

static int32_t hexdigit(char c) {
    if(c>='0' && c<='9')
        return c - '0';
    if(c>='A' && c<='F')
        return c - ('A'-10);
    if(c>='a' && c<='f')
        return c - ('a'-10);
    return -1;
}

static const int8_t biDiClassNameLengths[U_CHAR_DIRECTION_COUNT+1]={
    1, 1, 2, 2, 2, 2, 2, 1, 1, 2, 2, 3, 3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 0
};

UBool BiDiConformanceTest2::parseInputStringFromBiDiClasses(const char *&start) {
    inputString.remove();
    /*
     * Lengthy but fast BiDi class parser.
     * A simple parser could terminate or extract the name string and use
     *   int32_t biDiClassInt=u_getPropertyValueEnum(UCHAR_BIDI_CLASS, bidiClassString);
     * but that makes this test take significantly more time.
     */
    while(*start!=0 && *(start=u_skipWhitespace(start))!=0 && *start!=';') {
        UCharDirection biDiClass=U_CHAR_DIRECTION_COUNT;
        int32_t d1, d2, hexnum;
        // First look for an hexa value of at least 2 digits
        if((d1=hexdigit(start[0]))>=0 && (d2=hexdigit(start[1]))>=0) {
            const char *saveStart=start;
            hexnum=(d1<<4) + d2;
            start+=2;
            while((d1=hexdigit(start[0]))>=0) {
                hexnum=(hexnum<<4) + d1;
                start++;
            }
            if(hexnum<=0 || hexnum>0xffff ||
               (!U_IS_INV_WHITESPACE(start[0]) && start[0]!=';' && start[0]!=0)) {
                errln("\nError on line %d: Invalid hexa number at %s", (int)lineNumber, saveStart);
                return FALSE;
            }
            inputString.append(hexnum);
            continue;
        }
        // Compare each character once until we have a match on
        // a complete, short BiDi class name.
        if(start[0]=='L') {
            if(start[1]=='R') {
                if(start[2]=='E') {
                    biDiClass=U_LEFT_TO_RIGHT_EMBEDDING;
                } else if(start[2]=='I') {
                    biDiClass=U_LEFT_TO_RIGHT_ISOLATE;
                } else if(start[2]=='O') {
                    biDiClass=U_LEFT_TO_RIGHT_OVERRIDE;
                }
            } else {
                biDiClass=U_LEFT_TO_RIGHT;
            }
        } else if(start[0]=='R') {
            if(start[1]=='L') {
                if(start[2]=='E') {
                    biDiClass=U_RIGHT_TO_LEFT_EMBEDDING;
                } else if(start[2]=='I') {
                    biDiClass=U_RIGHT_TO_LEFT_ISOLATE;
                } else if(start[2]=='O') {
                    biDiClass=U_RIGHT_TO_LEFT_OVERRIDE;
                }
            } else {
                biDiClass=U_RIGHT_TO_LEFT;
            }
        } else if(start[0]=='E') {
            if(start[1]=='N') {
                biDiClass=U_EUROPEAN_NUMBER;
            } else if(start[1]=='S') {
                biDiClass=U_EUROPEAN_NUMBER_SEPARATOR;
            } else if(start[1]=='T') {
                biDiClass=U_EUROPEAN_NUMBER_TERMINATOR;
            }
        } else if(start[0]=='A') {
            if(start[1]=='L') {
                biDiClass=U_RIGHT_TO_LEFT_ARABIC;
            } else if(start[1]=='N') {
                biDiClass=U_ARABIC_NUMBER;
            }
        } else if(start[0]=='C' && start[1]=='S') {
            biDiClass=U_COMMON_NUMBER_SEPARATOR;
        } else if(start[0]=='B') {
            if(start[1]=='N') {
                biDiClass=U_BOUNDARY_NEUTRAL;
            } else {
                biDiClass=U_BLOCK_SEPARATOR;
            }
        } else if(start[0]=='S') {
            biDiClass=U_SEGMENT_SEPARATOR;
        } else if(start[0]=='W' && start[1]=='S') {
            biDiClass=U_WHITE_SPACE_NEUTRAL;
        } else if(start[0]=='O' && start[1]=='N') {
            biDiClass=U_OTHER_NEUTRAL;
        } else if(start[0]=='P' && start[1]=='D') {
            if(start[2]=='F') {
                biDiClass=U_POP_DIRECTIONAL_FORMAT;
            } else if(start[2]=='I') {
                biDiClass=U_POP_DIRECTIONAL_ISOLATE;
            }
        } else if(start[0]=='N' && start[1]=='S' && start[2]=='M') {
            biDiClass=U_DIR_NON_SPACING_MARK;
        } else if(start[0]=='F' && start[1]=='S' && start[2]=='I') {
            biDiClass=U_FIRST_STRONG_ISOLATE;
        }
        // Now we verify that the class name is terminated properly,
        // and not just the start of a longer word.
        int8_t biDiClassNameLength=biDiClassNameLengths[biDiClass];
        char c=start[biDiClassNameLength];
        if(biDiClass<U_CHAR_DIRECTION_COUNT && (U_IS_INV_WHITESPACE(c) || c==';' || c==0)) {
            inputString.append(charFromBiDiClass[biDiClass]);
            start+=biDiClassNameLength;
            continue;
        }
        // Accept any single character
        if(U_IS_INV_WHITESPACE(start[1]) || start[1]==';' || start[1]==0) {
            inputString.append(start[0]);
            start++;
            continue;
        }
        errln("\nError on line %d: BiDi class string not recognized at %s", (int)lineNumber, start);
        return FALSE;
    }
    return TRUE;
}

void BiDiConformanceTest2::TestBidiTest() {
    IcuTestErrorCode errorCode(*this, "TestBidiTest");
    const char *sourceTestDataPath=getSourceTestData(errorCode);
    if(errorCode.logIfFailureAndReset("unable to find the source/test/testdata "
                                      "folder (getSourceTestData())")) {
        return;
    }
    char bidiTestPath[400];
    strcpy(bidiTestPath, sourceTestDataPath);
    strcat(bidiTestPath, "BidiCharacterTest.txt");
    LocalStdioFilePointer bidiTestFile(fopen(bidiTestPath, "r"));
    if(bidiTestFile.isNull()) {
        errln("unable to open %s", bidiTestPath);
        return;
    }
    LocalUBiDiPointer ubidi(ubidi_open());
    lineNumber=0;
    levelsCount=0;
    orderingCount=0;
    errorCount=0;
    while(errorCount<10 && fgets(line, (int)sizeof(line), bidiTestFile.getAlias())!=NULL) {
        ++lineNumber;
        // Remove trailing comments and whitespace.
        char *commentStart=strchr(line, '#');
        if(commentStart!=NULL) {
            *commentStart=0;
        }
        u_rtrim(line);
        const char *start=u_skipWhitespace(line);
        if(*start==0) {
            continue;  // Skip empty and comment-only lines.
        }
        if(!parseInputStringFromBiDiClasses(start)) {
            errorCount++;
            continue;
        }
        start=u_skipWhitespace(start);
        if(*start!=';') {
            errorCount++;
            errln("\nError on line %d: Missing ; separator on line: %s", (int)lineNumber, line);
            continue;
        }
        start=u_skipWhitespace(start+1);
        char *end;
        uint32_t paraDirection=(uint32_t)strtoul(start, &end, 16);
        if(end<=start || (!U_IS_INV_WHITESPACE(*end) && *end!=';' && *end!=0) ||
                         paraDirection>3) {
            errorCount++;
            errln("\nError on line %d: Input paragraph direction incorrect at %s", (int)lineNumber, start);
            continue;
        }
        start=u_skipWhitespace(end);
        if(*start!=';') {
            errorCount++;
            errln("\nError on line %d: Missing ; separator on line: %s", (int)lineNumber, line);
            continue;
        }
        start++;
        uint32_t resolvedParaLevel=(uint32_t)strtoul(start, &end, 16);
        if(end<=start || (!U_IS_INV_WHITESPACE(*end) && *end!=';' && *end!=0) ||
           resolvedParaLevel>1) {
            errorCount++;
            errln("\nError on line %d: Resolved paragraph level incorrect at %s", (int)lineNumber, start);
            continue;
        }
        start=u_skipWhitespace(end);
        if(*start!=';') {
            errorCount++;
            errln("\nError on line %d: Missing ; separator on line: %s", (int)lineNumber, line);
            return;
        }
        start++;
        if(!parseLevels(start)) {
            errorCount++;
            continue;
        }
        start=u_skipWhitespace(start);
        if(*start==';') {
            if(!parseOrdering(start+1)) {
                errorCount++;
                continue;
            }
        }
        else
            orderingCount=-1;

        static const UBiDiLevel paraLevels[]={ 0, 1, UBIDI_DEFAULT_LTR, UBIDI_DEFAULT_RTL };
        static const char *const paraLevelNames[]={ "LTR", "RTL", "auto/LTR", "auto/RTL" };
        ubidi_setPara(ubidi.getAlias(), inputString.getBuffer(), inputString.length(),
                      paraLevels[paraDirection], NULL, errorCode);
        const UBiDiLevel *actualLevels=ubidi_getLevels(ubidi.getAlias(), errorCode);
        if(errorCode.logIfFailureAndReset("ubidi_setPara() or ubidi_getLevels()")) {
            errln("Input line %d: %s", (int)lineNumber, line);
            continue;
        }
        UBiDiLevel actualLevel;
        if((actualLevel=ubidi_getParaLevel(ubidi.getAlias()))!=resolvedParaLevel) {
            printErrorLine(paraLevelNames[paraDirection]);
            errln("\nError on line %d: Wrong resolved paragraph level; expected %d actual %d",
                   (int)lineNumber, resolvedParaLevel, actualLevel);
            continue;
        }
        if(!checkLevels(actualLevels, ubidi_getProcessedLength(ubidi.getAlias()),
                        paraLevelNames[paraDirection])) {
            continue;
        }
        if(orderingCount>=0 && !checkOrdering(ubidi.getAlias(), paraLevelNames[paraDirection])) {
            continue;
        }
    }
}

static UChar printLevel(UBiDiLevel level) {
    if(level<UBIDI_DEFAULT_LTR) {
        return 0x30+level;
    } else {
        return 0x78;  // 'x'
    }
}

static uint32_t getDirectionBits(const UBiDiLevel actualLevels[], int32_t actualCount) {
    uint32_t actualDirectionBits=0;
    for(int32_t i=0; i<actualCount; ++i) {
        actualDirectionBits|=(1<<(actualLevels[i]&1));
    }
    return actualDirectionBits;
}

UBool BiDiConformanceTest2::checkLevels(const UBiDiLevel actualLevels[], int32_t actualCount,
                                       const char *paraLevelName) {
    UBool isOk=TRUE;
    if(levelsCount!=actualCount) {
        errln("\nError on line %d: Wrong number of level values; expected %d actual %d",
              (int)lineNumber, (int)levelsCount, (int)actualCount);
        isOk=FALSE;
    } else {
        for(int32_t i=0; i<actualCount; ++i) {
            if(levels[i]!=actualLevels[i] && levels[i]<UBIDI_DEFAULT_LTR) {
                if(directionBits!=3 && directionBits==getDirectionBits(actualLevels, actualCount)) {
                    // ICU used a shortcut:
                    // Since the text is unidirectional, it did not store the resolved
                    // levels but just returns all levels as the paragraph level 0 or 1.
                    // The reordering result is the same, so this is fine.
                    break;
                } else {
                    errln("\nError on line %d: Wrong level value at index %d; expected %d actual %d",
                          (int)lineNumber, (int)i, levels[i], actualLevels[i]);
                    isOk=FALSE;
                    break;
                }
            }
        }
    }
    if(!isOk) {
        printErrorLine(paraLevelName);
        UnicodeString els("Expected levels:   ");
        int32_t i;
        for(i=0; i<levelsCount; ++i) {
            els.append((UChar)0x20).append(printLevel(levels[i]));
        }
        UnicodeString als("Actual   levels:   ");
        for(i=0; i<actualCount; ++i) {
            als.append((UChar)0x20).append(printLevel(actualLevels[i]));
        }
        errln(els);
        errln(als);
    }
    return isOk;
}

// Note: ubidi_setReorderingOptions(ubidi, UBIDI_OPTION_REMOVE_CONTROLS);
// does not work for custom BiDi class assignments
// and anyway also removes LRM/RLM/ZWJ/ZWNJ which is not desirable here.
// Therefore we just skip the indexes for BiDi controls while comparing
// with the expected ordering that has them omitted.
UBool BiDiConformanceTest2::checkOrdering(UBiDi *ubidi, const char *paraLevelName) {
    UBool isOk=TRUE;
    IcuTestErrorCode errorCode(*this, "TestBidiTest/checkOrdering()");
    int32_t resultLength=ubidi_getResultLength(ubidi);  // visual length including BiDi controls
    int32_t i, visualIndex;
    // Note: It should be faster to call ubidi_countRuns()/ubidi_getVisualRun()
    // and loop over each run's indexes, but that seems unnecessary for this test code.
    for(i=visualIndex=0; i<resultLength; ++i) {
        int32_t logicalIndex=ubidi_getLogicalIndex(ubidi, i, errorCode);
        if(errorCode.logIfFailureAndReset("ubidi_getLogicalIndex()")) {
            errln("Input line %d: %s", (int)lineNumber, line);
            return FALSE;
        }
        if(levels[logicalIndex]>=UBIDI_DEFAULT_LTR) {
            continue;  // BiDi control, omitted from expected ordering.
        }
        if(visualIndex<orderingCount && logicalIndex!=ordering[visualIndex]) {
            errln("\nError on line %d: Wrong ordering value at visual index %d; expected %d actual %d",
                  (int)lineNumber, (int)visualIndex, ordering[visualIndex], logicalIndex);
            isOk=FALSE;
            break;
        }
        ++visualIndex;
    }
    // visualIndex is now the visual length minus the BiDi controls,
    // which should match the length of the BidiTest.txt ordering.
    if(isOk && orderingCount!=visualIndex) {
        errln("\nError on line %d: Wrong number of ordering values; expected %d actual %d",
              (int)lineNumber, (int)orderingCount, (int)visualIndex);
        isOk=FALSE;
    }
    if(!isOk) {
        printErrorLine(paraLevelName);
        UnicodeString eord("Expected ordering: ");
        for(i=0; i<orderingCount; ++i) {
            eord.append((UChar)0x20).append((UChar)(0x30+ordering[i]));
        }
        UnicodeString aord("Actual   ordering: ");
        for(i=0; i<resultLength; ++i) {
            int32_t logicalIndex=ubidi_getLogicalIndex(ubidi, i, errorCode);
            if(levels[logicalIndex]<UBIDI_DEFAULT_LTR) {
                aord.append((UChar)0x20).append((UChar)(0x30+logicalIndex));
            }
        }
        errln(eord);
        errln(aord);
    }
    return isOk;
}

void BiDiConformanceTest2::printErrorLine(const char *paraLevelName) {
    ++errorCount;
    errln("Input line %5d:   %s", (int)lineNumber, line);
    errln(UnicodeString("Input string:       ")+inputString);
    errln("Para level:         %s", paraLevelName);
}