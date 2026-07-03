class SunshineVersion {
  constructor(release = null, version = null) {
    if (release) {
      this.release = release;
      this.version = release.tag_name;
      this.versionName = release.name;
      this.versionTag = release.tag_tag;
    } else if (version) {
      this.release = null;
      this.version = version;
      this.versionName = null;
      this.versionTag = null;
    } else {
      throw new Error('Either release or version must be provided');
    }
    this.versionParts = this.parseVersion(this.version);
    this.versionMajor = this.versionParts ? this.versionParts[0] : null;
    this.versionMinor = this.versionParts ? this.versionParts[1] : null;
    this.versionPatch = this.versionParts ? this.versionParts[2] : null;
  }

  parseVersion(version) {
    if (!version) {
      return null;
    }
    let v = version;
    if (v.indexOf("v") === 0) {
      v = v.substring(1);
    }
    return v.split('.').map(Number);
  }

  isGreater(otherVersion) {
    let otherVersionParts;
    if (otherVersion instanceof SunshineVersion) {
      otherVersionParts = otherVersion.versionParts;
    } else if (typeof otherVersion === 'string') {
      otherVersionParts = this.parseVersion(otherVersion);
    } else {
      throw new Error('Invalid argument: otherVersion must be a SunshineVersion object or a version string');
    }

    if (!this.versionParts || !otherVersionParts) {
      return false;
    }
    for (let i = 0; i < Math.min(3, this.versionParts.length, otherVersionParts.length); i++) {
      if (this.versionParts[i] !== otherVersionParts[i]) {
        return this.versionParts[i] > otherVersionParts[i];
      }
    }
    return false;
  }
}

/**
 * SolarFlare update-checker. Pulls the latest release from the
 * vindeckyy/Solar-Flare GitHub repo (24-hour localStorage cache, so
 * navigating between tabs doesn't re-hit GitHub), compares to the
 * installed version, and exposes the latest release body so the UI
 * can render release notes without each page re-fetching.
 *
 * The check is split from index.html so other pages (Navbar etc.)
 * can show the same "outdated" badge without duplicating the fetch.
 */
const UPDATE_CACHE_KEY = 'solarflare.update-check.v1';
const UPDATE_CACHE_TTL_MS = 24 * 60 * 60 * 1000;
const UPDATE_REPO = 'vindeckyy/Solar-Flare';
const UPDATE_API_URL = `https://api.github.com/repos/${UPDATE_REPO}/releases/latest`;

/**
 * Cached update-check payload (latest release + installed comparison).
 *
 * @typedef {Object} UpdateInfo
 * @property {string} latestVersion  e.g. "v0.1.7"
 * @property {string} htmlUrl        release page on GitHub
 * @property {string} releaseNotes   Markdown release body
 * @property {string} publishedAt    ISO timestamp
 * @property {string} checkedAt      ISO timestamp of the check itself
 * @property {boolean} outdated      true if latestVersion > installed
 */

/**
 * Compare the latest GitHub release against the installed version and
 * return an @c UpdateInfo. Caches in localStorage for 24 h so the
 * GitHub API rate-limit isn't hammered on every page nav.
 *
 * @param {string} installedVersion - The version string from /api/config.
 * @returns {Promise<UpdateInfo|null>} null if the API call failed.
 */
export async function checkForUpdate(installedVersion) {
  // Try the cache first; ignore stale-but-present entries because a
  // backgrounded refresh is fine to skip until the user reloads.
  const cached = readCache(installedVersion);
  if (cached) {
    // Kick off a background refresh but return the stale value now so
    // the UI never sits blank on first paint after the 24-h mark.
    refreshInBackground(installedVersion);
    return cached;
  }

  try {
    const release = await fetch(UPDATE_API_URL).then((r) => r.json());
    if (!release || !release.tag_name) {
      return null;
    }
    const info = buildUpdateInfo(release, installedVersion);
    writeCache(info, installedVersion);
    return info;
  } catch (e) {
    console.error('SolarFlare update check failed:', e);
    return null;
  }
}

function refreshInBackground(installedVersion) {
  // Fire-and-forget; we don't await so the caller renders immediately.
  fetch(UPDATE_API_URL).then((r) => r.json()).then((release) => {
    if (!release || !release.tag_name) return;
    writeCache(buildUpdateInfo(release, installedVersion), installedVersion);
  }).catch(() => { /* silent on background refresh failure */ });
}

function buildUpdateInfo(release, installedVersion) {
  const latest = new SunshineVersion(release, null);
  const installed = new SunshineVersion(null, installedVersion || 'v0.0.0');
  // Strip the leading 'v' from both so the rendered title is consistent
  // ("Running 0.0.5, latest is 9.9.9-scary-test" not "Running 0.0.5, latest is v9.9.9").
  // The SunshineVersion instance preserves the original; we just don't
  // want to double-prefix the rendered string.
  const stripV = (s) => (s || '').replace(/^v/i, '');
  return {
    latestVersion: stripV(latest.version),
    htmlUrl: release.html_url,
    releaseNotes: release.body || '',
    publishedAt: release.published_at || '',
    checkedAt: new Date().toISOString(),
    outdated: latest.isGreater(installed),
  };
}

function readCache(installedVersion) {
  try {
    const raw = localStorage.getItem(UPDATE_CACHE_KEY);
    if (!raw) return null;
    const cached = JSON.parse(raw);
    if (cached.installedVersion !== installedVersion) {
      // Installed version changed (e.g. user just upgraded). The cached
      // comparison is no longer meaningful.
      return null;
    }
    if (Date.now() - new Date(cached.info.checkedAt).getTime() > UPDATE_CACHE_TTL_MS) {
      return null;
    }
    return cached.info;
  } catch (e) {
    return null;
  }
}

function writeCache(info, installedVersion) {
  try {
    localStorage.setItem(UPDATE_CACHE_KEY, JSON.stringify({ info, installedVersion }));
  } catch (e) {
    // Quota errors etc. -- not fatal, the next page load will retry.
  }
}

export default SunshineVersion;
