/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2002-2013, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * UCAConformanceTest performs conformance tests defined in the data
 * files. ICU ships with stub data files, as the whole test are too 
 * long. To do the whole test, download the test files.
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "ucaconf.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "cstring.h"
#include "uparse.h"

#include "collationroot.h"  // TODO: Temporarily for v2 testing
#include "rulebasedcollator.h"  // TODO: Temporarily for v2 testing

UCAConformanceTest::UCAConformanceTest() :
rbUCA(NULL),
testFile(NULL),
status(U_ZERO_ERROR)
{
    UCA = (RuleBasedCollator *)Collator::createInstance(Locale::getRoot(), status);
    if(U_FAILURE(status)) {
        dataerrln("Error - UCAConformanceTest: Unable to open UCA collator! - %s", u_errorName(status));
    }

    CollationRoot::getData(status);
    if(U_FAILURE(status)) {
        errln("ERROR - UCAConformanceTest: Unable to open CLDR root collator!");
    }

    const char *srcDir = IntlTest::getSourceTestData(status);
    if (U_FAILURE(status)) {
        dataerrln("Could not open test data %s", u_errorName(status));
        return;
    }
    uprv_strcpy(testDataPath, srcDir);
    uprv_strcat(testDataPath, "CollationTest_");

    UVersionInfo uniVersion;
    static const UVersionInfo v62 = { 6, 2, 0, 0 };
    u_getUnicodeVersion(uniVersion);
    isAtLeastUCA62 = uprv_memcmp(uniVersion, v62, 4) >= 0;
}

UCAConformanceTest::~UCAConformanceTest()
{
    delete UCA;
    delete rbUCA;
    fclose(testFile);
}

void UCAConformanceTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par */)
{
    if(exec) {
        logln("TestSuite UCAConformanceTest: ");
    }
    TESTCASE_AUTO_BEGIN;
    // TODO: reenable TESTCASE_AUTO(TestTableNonIgnorable);
    // TODO: reenable TESTCASE_AUTO(TestTableShifted);
    // TODO: reenable TESTCASE_AUTO(TestRulesNonIgnorable);
    // TODO: reenable TESTCASE_AUTO(TestRulesShifted);
    TESTCASE_AUTO(TestTable2NonIgnorable);
    TESTCASE_AUTO(TestTable2Shifted);
    TESTCASE_AUTO_END;
}

void UCAConformanceTest::initRbUCA() 
{
    if(!rbUCA) {
        if (UCA) {
            UnicodeString ucarules;
            UCA->getRules(UCOL_FULL_RULES, ucarules);
            rbUCA = new RuleBasedCollator(ucarules, status);
            if (U_FAILURE(status)) {
                dataerrln("Failure creating UCA rule-based collator: %s", u_errorName(status));
                return;
            }
        } else {
            dataerrln("Failure creating UCA rule-based collator: %s", u_errorName(status));
            return;
        }
    }
}

void UCAConformanceTest::setCollNonIgnorable(Collator *coll) 
{
    coll->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    coll->setAttribute(UCOL_CASE_FIRST, UCOL_OFF, status);
    coll->setAttribute(UCOL_CASE_LEVEL, UCOL_OFF, status);
    coll->setAttribute(UCOL_STRENGTH, isAtLeastUCA62 ? UCOL_IDENTICAL : UCOL_TERTIARY, status);
    coll->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_NON_IGNORABLE, status);
}

void UCAConformanceTest::setCollShifted(Collator *coll) 
{
    coll->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    coll->setAttribute(UCOL_CASE_FIRST, UCOL_OFF, status);
    coll->setAttribute(UCOL_CASE_LEVEL, UCOL_OFF, status);
    coll->setAttribute(UCOL_STRENGTH, isAtLeastUCA62 ? UCOL_IDENTICAL : UCOL_QUATERNARY, status);
    coll->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
}

void UCAConformanceTest::openTestFile(const char *type)
{
    const char *ext = ".txt";
    if(testFile) {
        fclose(testFile);
    }
    char buffer[1024];
    uprv_strcpy(buffer, testDataPath);
    uprv_strcat(buffer, type);
    int32_t bufLen = (int32_t)uprv_strlen(buffer);

    // we try to open 3 files:
    // path/CollationTest_type.txt
    // path/CollationTest_type_SHORT.txt
    // path/CollationTest_type_STUB.txt
    // we are going to test with the first one that we manage to open.

    uprv_strcpy(buffer+bufLen, ext);

    testFile = fopen(buffer, "rb");

    if(testFile == 0) {
        uprv_strcpy(buffer+bufLen, "_SHORT");
        uprv_strcat(buffer, ext);
        testFile = fopen(buffer, "rb");

        if(testFile == 0) {
            uprv_strcpy(buffer+bufLen, "_STUB");
            uprv_strcat(buffer, ext);
            testFile = fopen(buffer, "rb");

            if (testFile == 0) {
                *(buffer+bufLen) = 0;
                dataerrln("Could not open any of the conformance test files, tried opening base %s\n", buffer);
                return;        
            } else {
                infoln(
                    "INFO: Working with the stub file.\n"
                    "If you need the full conformance test, please\n"
                    "download the appropriate data files from:\n"
                    "http://source.icu-project.org/repos/icu/tools/trunk/unicodetools/com/ibm/text/data/");
            }
        }
    }
}

static const uint32_t IS_SHIFTED = 1;
static const uint32_t FROM_RULES = 2;

static UBool
skipLineBecauseOfBug(const UChar *s, int32_t length, uint32_t flags) {
    // TODO: Fix ICU ticket #8052
    if(length >= 3 &&
            (s[0] == 0xfb2 || s[0] == 0xfb3) &&
            s[1] == 0x334 &&
            (s[2] == 0xf73 || s[2] == 0xf75 || s[2] == 0xf81)) {
        return TRUE;
    }
    // TODO: Fix ICU ticket #9361
    if((flags & IS_SHIFTED) != 0 && length >= 2 && s[0] == 0xfffe) {
        return TRUE;
    }
    // TODO: Fix tailoring builder, ICU ticket #9593.
    UChar c;
    if((flags & FROM_RULES) != 0 && length >= 2 && ((c = s[1]) == 0xedc || c == 0xedd)) {
        return TRUE;
    }
    return FALSE;
}

static UCollationResult
normalizeResult(int32_t result) {
    return result<0 ? UCOL_LESS : result==0 ? UCOL_EQUAL : UCOL_GREATER;
}

void UCAConformanceTest::testConformance(const Collator *coll) 
{
    if(testFile == 0) {
        return;
    }
    uint32_t skipFlags = 0;
    if(coll->getAttribute(UCOL_ALTERNATE_HANDLING, status) == UCOL_SHIFTED) {
        skipFlags |= IS_SHIFTED;
    }
    if(coll == rbUCA) {
        skipFlags |= FROM_RULES;
    }

    int32_t line = 0;

    UChar b1[1024], b2[1024];
    UChar *buffer = b1, *oldB = NULL;

    char lineB1[1024], lineB2[1024];
    char *lineB = lineB1, *oldLineB = lineB2;

    uint8_t sk1[1024], sk2[1024];
    uint8_t *oldSk = NULL, *newSk = sk1;

    int32_t oldLen = 0;
    int32_t oldBlen = 0;
    uint32_t first = 0;

    while (fgets(lineB, 1024, testFile) != NULL) {
        // remove trailing whitespace
        u_rtrim(lineB);

        line++;
        if(*lineB == 0 || lineB[0] == '#') {
            continue;
        }
        int32_t buflen = u_parseString(lineB, buffer, 1024, &first, &status);
        if(U_FAILURE(status)) {
            errln("Error parsing line %ld (%s): %s\n",
                  (long)line, u_errorName(status), lineB);
            status = U_ZERO_ERROR;
        }
        buffer[buflen] = 0;

        // TODO: Update conformance test files for UCA 6.3
        // where U+FFFD has the third-highest primary weight.
        if(buflen != 0 && buffer[0] == 0xfffd) {
            continue;
        }
        if(skipLineBecauseOfBug(buffer, buflen, skipFlags) && dynamic_cast<const RuleBasedCollator2 *>(coll) == NULL /* TODO: remove */) {
            logln("Skipping line %i because of a known bug", line);
            continue;
        }

        int32_t resLen = coll->getSortKey(buffer, buflen, newSk, 1024);

        if(oldSk != NULL) {
            UBool ok=TRUE;
            int32_t skres = strcmp((char *)oldSk, (char *)newSk);
            int32_t cmpres = coll->compare(oldB, oldBlen, buffer, buflen, status);
            int32_t cmpres2 = coll->compare(buffer, buflen, oldB, oldBlen, status);

            if(cmpres != -cmpres2) {
                errln("Compare result not symmetrical on line %i: "
                      "previous vs. current (%d) / current vs. previous (%d)",
                      line, cmpres, cmpres2);
                ok = FALSE;
            }

            // TODO: Compare with normalization turned off if the input passes the FCD test.

            if(cmpres != normalizeResult(skres)) {
                errln("Difference between coll->compare (%d) and sortkey compare (%d) on line %i",
                      cmpres, skres, line);
                ok = FALSE;
            }

            int32_t res = cmpres;
            if(res == 0 && !isAtLeastUCA62) {
                // Up to UCA 6.1, the collation test files use a custom tie-breaker,
                // comparing the raw input strings.
                res = u_strcmpCodePointOrder(oldB, buffer);
                // Starting with UCA 6.2, the collation test files use the standard UCA tie-breaker,
                // comparing the NFD versions of the input strings,
                // which we do via setting strength=identical.
            }
            if(res > 0) {
                errln("Line %i is not greater or equal than previous line", line);
                ok = FALSE;
            }

            if(!ok) {
                errln("  Previous data line %s", oldLineB);
                errln("  Current data line  %s", lineB);
                UnicodeString oldS, newS;
                prettify(CollationKey(oldSk, oldLen), oldS);
                prettify(CollationKey(newSk, resLen), newS);
                errln("  Previous key: "+oldS);
                errln("  Current key:  "+newS);
            }
        }

        // swap buffers
        oldLineB = lineB;
        oldB = buffer;
        oldSk = newSk;
        if(lineB == lineB1) {
            lineB = lineB2;
            buffer = b2;
            newSk = sk2;
        } else {
            lineB = lineB1;
            buffer = b1;
            newSk = sk1;
        }
        oldLen = resLen;
        oldBlen = buflen;
    }
}

void UCAConformanceTest::TestTableNonIgnorable(/* par */) {
    if (U_FAILURE(status)) {
        dataerrln("Error running UCA Conformance Test: %s", u_errorName(status));
        return;
    }
    setCollNonIgnorable(UCA);
    openTestFile("NON_IGNORABLE");
    testConformance(UCA);
}

void UCAConformanceTest::TestTableShifted(/* par */) {
    if (U_FAILURE(status)) {
        dataerrln("Error running UCA Conformance Test: %s", u_errorName(status));
        return;
    }
    setCollShifted(UCA);
    openTestFile("SHIFTED");
    testConformance(UCA);
}

void UCAConformanceTest::TestRulesNonIgnorable(/* par */) {
    initRbUCA();

    if(U_SUCCESS(status)) {
        setCollNonIgnorable(rbUCA);
        openTestFile("NON_IGNORABLE");
        testConformance(rbUCA);
    }
}

void UCAConformanceTest::TestRulesShifted(/* par */) {
    logln("This test is currently disabled, as it is impossible to "
        "wholly represent fractional UCA using tailoring rules.");
    return;

    initRbUCA();

    if(U_SUCCESS(status)) {
        setCollShifted(rbUCA);
        openTestFile("SHIFTED");
        testConformance(rbUCA);
    }
}

void UCAConformanceTest::TestTable2NonIgnorable() {
    LocalPointer<Collator> coll(CollationRoot::createCollator(status));
    setCollNonIgnorable(coll.getAlias());
    openTestFile("NON_IGNORABLE");
    testConformance(coll.getAlias());
}

void UCAConformanceTest::TestTable2Shifted() {
    LocalPointer<Collator> coll(CollationRoot::createCollator(status));
    setCollShifted(coll.getAlias());
    openTestFile("SHIFTED");
    testConformance(coll.getAlias());
}

#endif /* #if !UCONFIG_NO_COLLATION */