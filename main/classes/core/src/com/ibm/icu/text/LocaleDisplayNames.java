/*
 *******************************************************************************
 * Copyright (C) 2009-2014, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.text;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.text.CharacterIterator;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

import com.ibm.icu.impl.ICUConfig;
import com.ibm.icu.lang.UScript;
import com.ibm.icu.text.DisplayContext.Type;
import com.ibm.icu.util.ULocale;

/**
 * Returns display names of ULocales and components of ULocales. For
 * more information on language, script, region, variant, key, and
 * values, see {@link com.ibm.icu.util.ULocale}.
 * @stable ICU 4.4
 */
public abstract class LocaleDisplayNames {
    /**
     * Enum used in {@link #getInstance(ULocale, DialectHandling)}.
     * @stable ICU 4.4
     */
    public enum DialectHandling {
        /**
         * Use standard names when generating a locale name,
         * e.g. en_GB displays as 'English (United Kingdom)'.
         * @stable ICU 4.4
         */
        STANDARD_NAMES,
        /**
         * Use dialect names when generating a locale name,
         * e.g. en_GB displays as 'British English'.
         * @stable ICU 4.4
         */
        DIALECT_NAMES
    }

    // factory methods
    /**
     * Convenience overload of {@link #getInstance(ULocale, DialectHandling)} that specifies
     * STANDARD dialect handling.
     * @param locale the display locale
     * @return a LocaleDisplayNames instance
     * @stable ICU 4.4
     */
    public static LocaleDisplayNames getInstance(ULocale locale) {
        return getInstance(locale, DialectHandling.STANDARD_NAMES);
    };

    /**
     * Convenience overload of {@link #getInstance(Locale, DisplayContext...)} that specifies
     * {@link DisplayContext#STANDARD_NAMES}.
     * @param locale the display JDK locale
     * @return a LocaleDisplayNames instance
     * @draft ICU 54
     * @provisional This API might change or be removed in a future release.
     */
    public static LocaleDisplayNames getInstance(Locale locale) {
        return getInstance(ULocale.forLocale(locale));
    };

    /**
     * Returns an instance of LocaleDisplayNames that returns names formatted for the provided locale,
     * using the provided dialectHandling.
     * @param locale the display locale
     * @param dialectHandling how to select names for locales
     * @return a LocaleDisplayNames instance
     * @stable ICU 4.4
     */
    public static LocaleDisplayNames getInstance(ULocale locale, DialectHandling dialectHandling) {
        LocaleDisplayNames result = null;
        if (FACTORY_DIALECTHANDLING != null) {
            try {
                result = (LocaleDisplayNames) FACTORY_DIALECTHANDLING.invoke(null,
                        locale, dialectHandling);
            } catch (InvocationTargetException e) {
                // fall through
            } catch (IllegalAccessException e) {
                // fall through
            }
        }
        if (result == null) {
            result = new LastResortLocaleDisplayNames(locale, dialectHandling);
        }
        return result;
    }

    /**
     * Returns an instance of LocaleDisplayNames that returns names formatted for the provided locale,
     * using the provided DisplayContext settings
     * @param locale the display locale
     * @param contexts one or more context settings (e.g. for dialect
     *              handling, capitalization, etc.
     * @return a LocaleDisplayNames instance
     * @stable ICU 51
     */
    public static LocaleDisplayNames getInstance(ULocale locale, DisplayContext... contexts) {
        LocaleDisplayNames result = null;
        if (FACTORY_DISPLAYCONTEXT != null) {
            try {
                result = (LocaleDisplayNames) FACTORY_DISPLAYCONTEXT.invoke(null,
                        locale, (Object[])contexts);
            } catch (InvocationTargetException e) {
                // fall through
            } catch (IllegalAccessException e) {
                // fall through
            }
        }
        if (result == null) {
            result = new LastResortLocaleDisplayNames(locale, contexts);
        }
        return result;
    }

    /**
     * Returns an instance of LocaleDisplayNames that returns names formatted for the provided JDK
     * locale, using the provided DisplayContext settings
     * @param locale the display JDK locale
     * @param contexts one or more context settings (e.g. for dialect
     *              handling, capitalization, etc.
     * @return a LocaleDisplayNames instance
     * @draft ICU 54
     * @provisional This API might change or be removed in a future release.
     */
    public static LocaleDisplayNames getInstance(Locale locale, DisplayContext... contexts) {
        return getInstance(ULocale.forLocale(locale), contexts);
    }

    // getters for state
    /**
     * Returns the locale used to determine the display names. This is not necessarily the same
     * locale passed to {@link #getInstance}.
     * @return the display locale
     * @stable ICU 4.4
     */
    public abstract ULocale getLocale();

    /**
     * Returns the dialect handling used in the display names.
     * @return the dialect handling enum
     * @stable ICU 4.4
     */
    public abstract DialectHandling getDialectHandling();

    /**
     * Returns the current value for a specified DisplayContext.Type.
     * @param type the DisplayContext.Type whose value to return
     * @return the current DisplayContext setting for the specified type
     * @stable ICU 51
     */
    public abstract DisplayContext getContext(DisplayContext.Type type);

    // names for entire locales
    /**
     * Returns the display name of the provided ulocale.
     * @param locale the locale whose display name to return
     * @return the display name of the provided locale
     * @stable ICU 4.4
     */
    public abstract String localeDisplayName(ULocale locale);

    /**
     * Returns the display name of the provided locale.
     * @param locale the locale whose display name to return
     * @return the display name of the provided locale
     * @stable ICU 4.4
     */
    public abstract String localeDisplayName(Locale locale);

    /**
     * Returns the display name of the provided locale id.
     * @param localeId the id of the locale whose display name to return
     * @return the display name of the provided locale
     * @stable ICU 4.4
     */
    public abstract String localeDisplayName(String localeId);

    // names for components of a locale id
    /**
     * Returns the display name of the provided language code.
     * @param lang the language code
     * @return the display name of the provided language code
     * @stable ICU 4.4
     */
    public abstract String languageDisplayName(String lang);

    /**
     * Returns the display name of the provided script code.
     * @param script the script code
     * @return the display name of the provided script code
     * @stable ICU 4.4
     */
    public abstract String scriptDisplayName(String script);

    /**
     * Returns the display name of the provided script code
     * when used in the context of a full locale name.
     * @param script the script code
     * @return the display name of the provided script code
     * @internal ICU 49
     * @deprecated This API is ICU internal only.
     */
    @Deprecated
    public String scriptDisplayNameInContext(String script) {
        return scriptDisplayName(script);
    }

    /**
     * Returns the display name of the provided script code.  See
     * {@link com.ibm.icu.lang.UScript} for recognized script codes.
     * @param scriptCode the script code number
     * @return the display name of the provided script code
     * @stable ICU 4.4
     */
    public abstract String scriptDisplayName(int scriptCode);

    /**
     * Returns the display name of the provided region code.
     * @param region the region code
     * @return the display name of the provided region code
     * @stable ICU 4.4
     */
    public abstract String regionDisplayName(String region);

    /**
     * Returns the display name of the provided variant.
     * @param variant the variant string
     * @return the display name of the provided variant
     * @stable ICU 4.4
     */
    public abstract String variantDisplayName(String variant);

    /**
     * Returns the display name of the provided locale key.
     * @param key the locale key name
     * @return the display name of the provided locale key
     * @stable ICU 4.4
     */
    public abstract String keyDisplayName(String key);

    /**
     * Returns the display name of the provided value (used with the provided key).
     * @param key the locale key name
     * @param value the locale key's value
     * @return the display name of the provided value
     * @stable ICU 4.4
     */
    public abstract String keyValueDisplayName(String key, String value);

    /**
     * Return a list of information used to construct a UI list of locale names.
     * @param collator how to collate—should normally be Collator.getInstance(locale)
     * @param inSelf if true, compares the nameInSelf, otherwise the nameInDisplayLocale.
     * Set depending on which field (displayLocale vs self) is to show up in the UI.
     * If both are to show up in the UI, then it should be the one used for the primary sort order.
     * @param localeSet a list of locales to present in a UI list. The casing uses the settings in the LocaleDisplayNames instance.
     * @return an ordered list of UiListItems.
     */
    public List<UiListItem> getUiList(Comparator<Object> collator, boolean inSelf, Collection<ULocale> localeSet) {
        return getUiListX(UiListItem.getComparator(collator, inSelf), localeSet);
    }

    /**
     * Return a list of information used to construct a UI list of locale names, providing more access to control the sorting. 
     * Normally use getUiList instead.
     * @param comparator how to sort the UiListItems in the result.
     * @param localeSet a list of locales to present in a UI list. The casing uses the settings in the LocaleDisplayNames instance.
     * @return an ordered list of UiListItems.
     */
    public abstract List<UiListItem> getUiListX(Comparator<UiListItem> comparator, Collection<ULocale> localeSet);

    /**
     * Struct-like class used to return information for constructing a UI list, each corresponding to a locale.
     */
    public static class UiListItem {
        /**
         * Returns the minimized locale for an input locale, such as sr-Cyrl → sr
         */
        public final ULocale minimized;
        /**
         * Returns the modified locale for an input locale, such as sr → sr-Cyrl, where there is also an sr-Latn in the list
         */
        public final ULocale modified;
        /**
         * Returns the name of the modified locale in the display locale, such as "Englisch (VS)" (for 'en-US', where the display locale is 'de').
         */
        public final String nameInDisplayLocale;
        /**
         * Returns the name of the modified locale in itself, such as "English (US)" (for 'en-US').
         */
        public final String nameInSelf;

        /** 
         * Constructor, normally only called internally.
         * @param nameInDisplayLocale name of the modified locale in the display locale
         * @param nameInSelf name of the modified locale in itself
         * @param minimized locale for an input locale
         * @param modified modified for an input locale
         */
        public UiListItem(String nameInDisplayLocale, String nameInSelf, ULocale minimized, ULocale modified) {
            this.minimized = minimized;
            this.modified = modified;
            this.nameInDisplayLocale = nameInDisplayLocale;
            this.nameInSelf = nameInSelf;
        }
        
        @Override
        public boolean equals(Object obj) {
            UiListItem other = (UiListItem)obj;
            return nameInDisplayLocale.equals(other.nameInDisplayLocale)
                    && nameInSelf.equals(other.nameInSelf)
                    && minimized.equals(other.minimized)
                    && modified.equals(other.modified);
        }
        
        @Override
        public int hashCode() {
            return modified.hashCode() ^ nameInDisplayLocale.hashCode();
        }
        
        @Override
        public String toString() {
            return "{" + nameInDisplayLocale + ", " + nameInSelf + ", " + modified + ", " + minimized + "}";
        }
        
        /**
         * Return a comparator that compares the locale names for the display locale or the in-self names, depending on an input parameter.
         * @param inSelf if true, compares the nameInSelf, otherwise the nameInDisplayLocale
         * @param comparator (meant for strings, but because Java Collator doesn't have &lt;String>...) 
         * @return UiListItem comparator
         */
        public static Comparator<UiListItem> getComparator(Comparator<Object> comparator, boolean inSelf) {
            return new UiListItemComparator(comparator, inSelf);
        }

        private static class UiListItemComparator implements Comparator<UiListItem> {
            private final Comparator<Object> collator;
            private final boolean useSelf;
            UiListItemComparator(Comparator<Object> collator, boolean useSelf) {
                this.collator = collator;
                this.useSelf = useSelf;
            }
            public int compare(UiListItem o1, UiListItem o2) {
                int result = useSelf ? collator.compare(o1.nameInSelf, o2.nameInSelf)
                        : collator.compare(o1.nameInDisplayLocale, o2.nameInDisplayLocale);
                return result != 0 ? result : o1.modified.compareTo(o2.modified); // just in case
            }
        }
    }

    /**
     * Sole constructor.  (For invocation by subclass constructors,
     * typically implicit.)
     * @internal
     * @deprecated This API is ICU internal only.
     */
    @Deprecated
    protected LocaleDisplayNames() {
    }

    private static final Method FACTORY_DIALECTHANDLING;
    private static final Method FACTORY_DISPLAYCONTEXT;

    static {
        String implClassName = ICUConfig.get("com.ibm.icu.text.LocaleDisplayNames.impl", "com.ibm.icu.impl.LocaleDisplayNamesImpl");

        Method factoryDialectHandling = null;
        Method factoryDisplayContext = null;

        try {
            Class<?> implClass = Class.forName(implClassName);
            try {
                factoryDialectHandling = implClass.getMethod("getInstance",
                        ULocale.class, DialectHandling.class);
            } catch (NoSuchMethodException e) {
            }
            try {
                factoryDisplayContext = implClass.getMethod("getInstance",
                        ULocale.class, DisplayContext[].class);
            } catch (NoSuchMethodException e) {
            }

        } catch (ClassNotFoundException e) {
            // fallback to last resort impl
        }

        FACTORY_DIALECTHANDLING = factoryDialectHandling;
        FACTORY_DISPLAYCONTEXT = factoryDisplayContext;
    }

    /**
     * Minimum implementation of LocaleDisplayNames
     */
    private static class LastResortLocaleDisplayNames extends LocaleDisplayNames {

        private ULocale locale;
        private DisplayContext[] contexts;

        private LastResortLocaleDisplayNames(ULocale locale, DialectHandling dialectHandling) {
            this.locale = locale;
            DisplayContext context = (dialectHandling == DialectHandling.DIALECT_NAMES) ?
                    DisplayContext.DIALECT_NAMES : DisplayContext.STANDARD_NAMES;
            this.contexts = new DisplayContext[] {context};
        }

        private LastResortLocaleDisplayNames(ULocale locale, DisplayContext... contexts) {
            this.locale = locale;
            this.contexts = new DisplayContext[contexts.length];
            System.arraycopy(contexts, 0, this.contexts, 0, contexts.length);
        }

        @Override
        public ULocale getLocale() {
            return locale;
        }

        @Override
        public DialectHandling getDialectHandling() {
            DialectHandling result = DialectHandling.STANDARD_NAMES;
            for (DisplayContext context : contexts) {
                if (context.type() == DisplayContext.Type.DIALECT_HANDLING) {
                    if (context.value() == DisplayContext.DIALECT_NAMES.ordinal()) {
                        result = DialectHandling.DIALECT_NAMES;
                        break;
                    }
                }
            }
            return result;
        }

        @Override
        public DisplayContext getContext(Type type) {
            DisplayContext result = DisplayContext.STANDARD_NAMES;  // final fallback
            for (DisplayContext context : contexts) {
                if (context.type() == type) {
                    result = context;
                    break;
                }
            }
            return result;
        }

        @Override
        public String localeDisplayName(ULocale locale) {
            return locale.getName();
        }

        @Override
        public String localeDisplayName(Locale locale) {
            return ULocale.forLocale(locale).getName();
        }

        @Override
        public String localeDisplayName(String localeId) {
            return new ULocale(localeId).getName();
        }

        @Override
        public String languageDisplayName(String lang) {
            return lang;
        }

        @Override
        public String scriptDisplayName(String script) {
            return script;
        }

        @Override
        public String scriptDisplayName(int scriptCode) {
            return UScript.getShortName(scriptCode);
        }

        @Override
        public String regionDisplayName(String region) {
            return region;
        }

        @Override
        public String variantDisplayName(String variant) {
            return variant;
        }

        @Override
        public String keyDisplayName(String key) {
            return key;
        }

        @Override
        public String keyValueDisplayName(String key, String value) {
            return value;
        }

        @Override
        public List<UiListItem> getUiListX(Comparator<UiListItem> comparator, Collection<ULocale> localeSet) {
            return Collections.emptyList();
        }
    }
    /**
     * Should move to being public.
     * @internal
     * @deprecated This API is ICU internal only.
     */
    public static final class StringBreakIterator extends BreakIterator {
        private final BreakIterator source;
        private final int[] breakpoints = new int[2];
        private int lastIndex;
        private int currentIndex;

        public StringBreakIterator(ULocale locale) {
            this.source = BreakIterator.getTitleInstance(locale);
            setup(source);
        }

        private void setup(BreakIterator source) {
            int last = source.last();
            breakpoints[0] = 0;
            breakpoints[1] = last;
            if (last == 0) {
                lastIndex = 0;
            } else {
                lastIndex = 1;
            }
            source.first();
        }

        @Override
        public int first() {
            return breakpoints[currentIndex = 0];
        }

        @Override
        public int last() {
            return breakpoints[currentIndex = lastIndex];
        }

        @Override
        public int next(int n) {
            return 0;
        }

        @Override
        public int next() {
            if (currentIndex >= lastIndex) {
                return DONE;
            }
            return breakpoints[++currentIndex];
        }

        @Override
        public int previous() {
            if (currentIndex <= 0) {
                return DONE;
            }
            return breakpoints[--currentIndex];
        }

        @Override
        public int following(int offset) {
            if (offset >= breakpoints[1]) {
                return DONE;
            } else if (offset >= breakpoints[0]) {
                currentIndex = 1;
            } else {
                currentIndex = 0;
            }
            return breakpoints[currentIndex];
        }

        @Override
        public int current() {
            return breakpoints[currentIndex];
        }

        @Override
        public CharacterIterator getText() {
            return source.getText();
        }

        @Override
        public void setText(CharacterIterator newText) {
            source.setText(newText);
            setup(source);
        }
    }
}