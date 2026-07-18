# Development Guide

The repository-level [contribution policy](../CONTRIBUTING.md) is the source of
truth for scope, review expectations, and fork boundaries. This page covers
the local development loop.

## Toolchain

SolarFlare uses CMake, Ninja, C++23, Node.js, Vue, Vite, and GoogleTest. The
platform dependency reference is in [Building](building.md). Keep all build
trees under a `cmake-build-` prefix.

On Windows, run build commands from MSYS2 UCRT64:

```powershell
C:\msys64\msys2_shell.cmd -defterm -here -no-start -ucrt64 -c "<command>"
```

## Web UI

The Web UI lives in `src_assets/common/assets/web`.

- `Navbar.vue` owns the shared desktop rail and compact mobile navigation.
- `sunshine.css` is the shared SolarFlare design system; its filename remains
  a compatibility path.
- `init.js` initializes the shared theme and locale state for every Vite entry.
- Vue components own interactive configuration surfaces.
- EJS templates provide the static page shells.

The interface deliberately avoids a client-side router. Keep endpoints and
form serialization independent from the visual shell. Motion must communicate
an interaction or state change, and every transition needs a usable
`prefers-reduced-motion` state.

Build the production bundle through CMake:

```bash
cmake -S . -B cmake-build-web -G Ninja -DBUILD_DOCS=OFF
cmake --build cmake-build-web --target web-ui -j2
```

For the Vite development server:

```bash
npm install
npm run dev
```

## Localization

English source strings belong only in
`src_assets/common/assets/web/public/assets/locale/en.json`. Do not edit
`en_US`, `en_GB`, or any other locale as part of an English UI change.

Use stable, descriptive keys. Product copy should say SolarFlare; internal
keys such as `sunshine_name` remain unchanged where renaming would break
configuration or translation compatibility.

## C++ Style and Documentation

Apply the repository `.clang-format` rules to changed C and C++ files. Every
new or modified function, type, member, and public constant must have Doxygen
documentation. Use the project form for primary comments:

```cpp
/**
 * @brief Describe the symbol.
 *
 * @param value Describe the parameter.
 * @return Describe the result.
 */
```

Use `///< ...` for inline member documentation.

## Tests

Add or update GoogleTest coverage for changed behavior and target full coverage
of new branches. Configure, build, and run the test executable with:

```bash
cmake -S . -B cmake-build-tests -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_DOCS=OFF
cmake --build cmake-build-tests --target test_sunshine -j2
./cmake-build-tests/tests/test_sunshine --gtest_brief=1
```

The `test_sunshine` filename is retained for build compatibility.

## Before Submitting

```bash
git diff --check
npm run build
./cmake-build-tests/tests/test_sunshine --gtest_brief=1
```

Document user-visible behavior and operational changes in the appropriate
SolarFlare guide. Submit changes to the `vindeckyy/Solar-Flare` repository;
do not open SolarFlare work in the LizardByte organization.

<div class="section_buttons">

| Previous                |                                                         Next |
|:------------------------|-------------------------------------------------------------:|
| [Building](building.md) | [Source Code](../third-party/doxyconfig/docs/source_code.md) |

</div>

<details style="display: none;">
  <summary></summary>
  [TOC]
</details>
