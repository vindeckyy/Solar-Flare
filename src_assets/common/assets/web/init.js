import i18n from './locale'
import { loadAutoTheme } from './theme'

import 'bootstrap/dist/css/bootstrap.min.css'
// Load the shared SolarFlare stylesheet after Bootstrap so theme rules win.
// Makes themes load and style correctly.
import './sunshine.css'

// must import even if not implicitly using here
// https://github.com/aurelia/skeleton-navigation/issues/894
// https://discourse.aurelia.io/t/bootstrap-import-bootstrap-breaks-dropdown-menu-in-navbar/641/9
import 'bootstrap/dist/js/bootstrap'

/**
 * @brief Initialize a SolarFlare web entry point with the shared theme and locale.
 *
 * @param app Vue application instance to initialize.
 * @param config Optional callback invoked after the application is mounted.
 * @return Nothing.
 */
export function initApp(app, config) {
    // Apply the color system before localization resolves so first-run and
    // authentication pages receive the same theme as the main host console.
    loadAutoTheme()

    // PWA: register the service worker (network-first for pages/API/unhashed
    // files, cache-first for Vite content-hashed assets). Ignore failures —
    // the UI works fine without it.
    if ('serviceWorker' in navigator) {
        window.addEventListener('load', () => {
            navigator.serviceWorker.register('./sw.js').catch(() => {})
        })
    }

    //Wait for locale initialization, then render
    i18n().then(i18n => {
        app.use(i18n);
        app.provide('i18n', i18n.global)
        app.mount('#app');
        if (config) {
            config(app)
        }
    });
}
