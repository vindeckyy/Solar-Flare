import {createI18n} from "vue-i18n";

// Import only the fallback language files
import en from './public/assets/locale/en.json'

/**
 * @brief Replace the upstream product name in localized presentation copy.
 *
 * Locale source files other than English remain synchronized with upstream,
 * so branding is normalized after loading instead of modifying every
 * translation catalog. Object keys and protocol/configuration identifiers are
 * deliberately preserved.
 *
 * @param value Localized string, array, or message object to normalize.
 * @return A copy of @p value whose user-visible product name is SolarFlare.
 */
function brandMessageTree(value) {
    if (typeof value === 'string') {
        return value
            .replaceAll(/\bSUNSHINE\b/g, 'SOLARFLARE')
            .replaceAll(/\bSunshine\b/g, 'SolarFlare')
            .replaceAll(/\bsunshine\b/g, 'solarflare')
    }
    if (Array.isArray(value)) {
        return value.map(brandMessageTree)
    }
    if (value && typeof value === 'object') {
        return Object.fromEntries(Object.entries(value).map(([key, message]) => [
            key,
            key === 'theme_sunshine' ? 'SolarFlare Daylight' : brandMessageTree(message)
        ]))
    }
    return value
}

export default async function() {
    let r = await (await fetch("./api/configLocale")).json();
    let locale = r.locale ?? "en";
    document.querySelector('html').setAttribute('lang', locale);
    let messages = {
        en: brandMessageTree(en)
    };
    try {
        if (locale !== 'en') {
            let r = await (await fetch(`./assets/locale/${locale}.json`)).json();
            messages[locale] = brandMessageTree(r);
        }
    } catch (e) {
        console.error("Failed to download translations", e);
    }
    const i18n = createI18n({
        locale: locale, // set locale
        fallbackLocale: 'en', // set fallback locale
        messages: messages
    })
    return i18n;
}
