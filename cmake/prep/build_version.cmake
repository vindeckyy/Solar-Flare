# Set build variables if env variables are defined
# These are used in configured files such as manifests for different packages
if(DEFINED ENV{BRANCH})
    set(GITHUB_BRANCH $ENV{BRANCH})
endif()
if(DEFINED ENV{BUILD_VERSION})  # cmake-lint: disable=W0106
    set(BUILD_VERSION $ENV{BUILD_VERSION})
endif()
if(DEFINED ENV{CLONE_URL})
    set(GITHUB_CLONE_URL $ENV{CLONE_URL})
endif()
if(DEFINED ENV{COMMIT})
    set(GITHUB_COMMIT $ENV{COMMIT})
endif()
if(DEFINED ENV{TAG})
    set(GITHUB_TAG $ENV{TAG})
endif()

# Check if env vars are defined before attempting to access them, variables will be defined even if blank
if((DEFINED ENV{BRANCH}) AND (DEFINED ENV{BUILD_VERSION}))  # cmake-lint: disable=W0106
    if((DEFINED ENV{BRANCH}) AND (NOT $ENV{BUILD_VERSION} STREQUAL ""))
        # If BRANCH is defined and BUILD_VERSION is not empty, then we are building from CI
        # If BRANCH is master we are building a push/release build
        MESSAGE("Got from CI '$ENV{BRANCH}' branch and version '$ENV{BUILD_VERSION}'")
        set(PROJECT_VERSION $ENV{BUILD_VERSION})
        string(REGEX REPLACE "^v" "" PROJECT_VERSION ${PROJECT_VERSION})  # remove the v prefix if it exists
        set(CMAKE_PROJECT_VERSION ${PROJECT_VERSION})  # cpack will use this to set the binary versions
    endif()
else()
    # Generate Sunshine Version based of the git tag
    # https://github.com/nocnokneo/cmake-git-versioning-example/blob/master/LICENSE
    find_package(Git)
    if(GIT_EXECUTABLE)
        MESSAGE("${CMAKE_SOURCE_DIR}")
        get_filename_component(SRC_DIR "${CMAKE_SOURCE_DIR}" DIRECTORY)
        #Get current Branch
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                OUTPUT_VARIABLE GIT_DESCRIBE_BRANCH
                RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # Gather current commit (short form, e.g. "a1b2c3d")
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
                RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # Gather current commit (long form, used for PROJECT_VERSION_COMMIT)
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
                OUTPUT_VARIABLE GIT_COMMIT_FULL
                RESULT_VARIABLE GIT_COMMIT_FULL_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # Gather closest semver-style tag if HEAD is on or near a release.
        # `git describe --tags` returns "v2026.708.0-solarflare" (clean on tag)
        # or "v2026.708.0-solarflare-1-g543eed3" (N commits past the tag).
        # Both forms already encode the release date (YYYY.DDD.N), so the
        # internal version string matches the GitHub release tag exactly.
        # Falls back to commit-count + short-SHA if no tag exists.
        execute_process(
                COMMAND ${GIT_EXECUTABLE} describe --tags --long HEAD
                OUTPUT_VARIABLE GIT_DESCRIBE_LONG
                RESULT_VARIABLE GIT_DESCRIBE_LONG_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # Strip "v" prefix and "-solarflare" suffix from the tag portion.
        # "v2026.708.0-solarflare-1-g543eed3" -> "2026.708.0-1-g543eed3"
        # "v2026.708.0-solarflare" -> "2026.708.0"
        string(REGEX REPLACE "^v|\-solarflare" "" GIT_DESCRIBE_RELEASE "${GIT_DESCRIBE_LONG}")
        # Gather total commit count, used as the version base so every
        # commit produces a unique, monotonically-increasing version string.
        # `git rev-list --count HEAD` returns N (the position of HEAD in
        # the linear commit history, 1-indexed). 0 is reserved for the
        # "git failed" sentinel.
        execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
                OUTPUT_VARIABLE GIT_REV_COUNT
                RESULT_VARIABLE GIT_REV_COUNT_ERROR
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # Check if Dirty
        execute_process(
                COMMAND ${GIT_EXECUTABLE} diff --quiet --exit-code
                RESULT_VARIABLE GIT_IS_DIRTY
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NOT GIT_DESCRIBE_ERROR_CODE)
            MESSAGE("Sunshine Branch: ${GIT_DESCRIBE_BRANCH}")

            # Use the git commit count as the version base so each commit
            # produces a unique, sortable version number. The short SHA is
            # appended so two builds off the same commit count (e.g. a
            # re-tagged release) still differ. Format:
            #   <count>-<short-sha>             # clean
            #   <count>-<short-sha>-dirty       # uncommitted changes
            # The default fallback (no git) is the hard-coded 2026.999.2
            # from CMakeLists.txt.
            if(NOT GIT_DESCRIBE_LONG_ERROR AND NOT "${GIT_DESCRIBE_RELEASE}" STREQUAL "")
                # Use the tag-based version. Format: YYYY.DDD.N or YYYY.DDD.N-M-g<sha>.
                set(PROJECT_VERSION "${GIT_DESCRIBE_RELEASE}")
                MESSAGE("Sunshine Version: ${PROJECT_VERSION} (tag-based)")
            elseif(NOT GIT_REV_COUNT_ERROR AND NOT "${GIT_REV_COUNT}" STREQUAL "")
                set(PROJECT_VERSION "${GIT_REV_COUNT}-${GIT_DESCRIBE_VERSION}")
                MESSAGE("Sunshine Version: ${PROJECT_VERSION} (commit count ${GIT_REV_COUNT})")
            else()
                set(PROJECT_VERSION "${GIT_DESCRIBE_VERSION}")
                MESSAGE("Sunshine Version: ${PROJECT_VERSION} (rev-list failed, short SHA only)")
            endif()

            if(GIT_IS_DIRTY)
                set(PROJECT_VERSION "${PROJECT_VERSION}-dirty")
                MESSAGE("Git tree is dirty!")
            endif()
        else()
            MESSAGE(ERROR ": Got git error while fetching tags: ${GIT_DESCRIBE_ERROR_CODE}")
        endif()
        # Fall back to git HEAD when COMMIT/GITHUB_COMMIT env vars aren't set
        # (local builds don't have them). PROJECT_VERSION_COMMIT is exposed in
        # src/main.cpp via the "Sunshine version: ... commit: ..." log line,
        # so leaving it blank prints a bare "commit: " which looks broken.
        if(NOT GIT_COMMIT_FULL_ERROR AND NOT "${GIT_COMMIT_FULL}" STREQUAL "")
            set(GITHUB_COMMIT "${GIT_COMMIT_FULL}")
        endif()
    else()
        MESSAGE(WARNING ": Git not found, cannot find git version")
    endif()
endif()

# set date variables
set(PROJECT_YEAR "1990")
set(PROJECT_MONTH "01")
set(PROJECT_DAY "01")

# Extract year, month, and day (do this AFTER version parsing)
# Note: Cmake doesn't support "{}" regex syntax
if(PROJECT_VERSION MATCHES "^([0-9][0-9][0-9][0-9])\\.([0-9][0-9][0-9][0-9]?)\\.([0-9]+)$")
    message("Extracting year and month/day from PROJECT_VERSION: ${PROJECT_VERSION}")
    # First capture group is the year
    set(PROJECT_YEAR "${CMAKE_MATCH_1}")

    # Second capture group contains month and day
    set(MONTH_DAY "${CMAKE_MATCH_2}")

    # Extract month (first 1-2 digits) and day (last 2 digits)
    string(LENGTH "${MONTH_DAY}" MONTH_DAY_LENGTH)
    if(MONTH_DAY_LENGTH EQUAL 3)
        # Format: MDD (e.g., 703 = month 7, day 03)
        string(SUBSTRING "${MONTH_DAY}" 0 1 PROJECT_MONTH)
        string(SUBSTRING "${MONTH_DAY}" 1 2 PROJECT_DAY)
    elseif(MONTH_DAY_LENGTH EQUAL 4)
        # Format: MMDD (e.g., 1203 = month 12, day 03)
        string(SUBSTRING "${MONTH_DAY}" 0 2 PROJECT_MONTH)
        string(SUBSTRING "${MONTH_DAY}" 2 2 PROJECT_DAY)
    endif()

    # Ensure month is two digits
    if(PROJECT_MONTH LESS 10 AND NOT PROJECT_MONTH MATCHES "^0")
        set(PROJECT_MONTH "0${PROJECT_MONTH}")
    endif()
    # Ensure day is two digits
    if(PROJECT_DAY LESS 10 AND NOT PROJECT_DAY MATCHES "^0")
        set(PROJECT_DAY "0${PROJECT_DAY}")
    endif()
else()
    # Fall back to the HEAD commit date for git-derived versions
    # ("<count>-<short-sha>[-dirty]" or just "<short-sha>"). The Windows
    # RC file's VERSIONINFO block needs a real year so the binary
    # version looks like a normal "2026.703" rather than the bogus
    # 1990 fallback.
    if(GIT_EXECUTABLE AND NOT PROJECT_VERSION MATCHES "^[0-9][0-9][0-9][0-9]\\..*")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} log -1 --format=%ci HEAD
            OUTPUT_VARIABLE GIT_COMMIT_DATE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(GIT_COMMIT_DATE MATCHES "^([0-9]{4})-([0-9]{2})-([0-9]{2})")
            set(PROJECT_YEAR "${CMAKE_MATCH_1}")
            set(PROJECT_MONTH "${CMAKE_MATCH_2}")
            set(PROJECT_DAY "${CMAKE_MATCH_3}")
            message("Extracted year/month/day from git HEAD date: ${PROJECT_YEAR}-${PROJECT_MONTH}-${PROJECT_DAY}")
        endif()
    endif()
endif()

# Parse PROJECT_VERSION to extract major, minor, and patch components
if(PROJECT_VERSION MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(PROJECT_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(CMAKE_PROJECT_VERSION_MAJOR "${CMAKE_MATCH_1}")

    set(PROJECT_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(CMAKE_PROJECT_VERSION_MINOR "${CMAKE_MATCH_2}")

    set(PROJECT_VERSION_PATCH "${CMAKE_MATCH_3}")
    set(CMAKE_PROJECT_VERSION_PATCH "${CMAKE_MATCH_3}")
endif()

# Fall back to sane defaults when PROJECT_VERSION isn't semver-shaped.
# Git-derived versions look like "4-1856aa4" or "4-1856aa4-dirty" and
# don't match the major.minor.patch regex above; downstream RC file
# generation still needs PROJECT_VERSION_MAJOR/MINOR/PATCH and the
# PROJECT_YEAR/MONTH/DAY for the VERSIONINFO block, so default those to
# the current year + zero values rather than the bogus 1990 default.
if(NOT PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR "0")
    set(CMAKE_PROJECT_VERSION_MAJOR "0")
endif()
if(NOT PROJECT_VERSION_MINOR)
    set(PROJECT_VERSION_MINOR "0")
    set(CMAKE_PROJECT_VERSION_MINOR "0")
endif()
if(NOT PROJECT_VERSION_PATCH)
    set(PROJECT_VERSION_PATCH "0")
    set(CMAKE_PROJECT_VERSION_PATCH "0")
endif()

# Split PROJECT_VERSION_PATCH for RC file (Windows VERSIONINFO requires values <= 65535)
# PROJECT_VERSION_PATCH can be 0-245959, so we split it into two parts:
# - Last 2 digits for RC_VERSION_REVISION
# - Leading digits for RC_VERSION_BUILD (0 if original is <= 99)
math(EXPR RC_VERSION_BUILD "${PROJECT_VERSION_PATCH} / 100")
math(EXPR RC_VERSION_REVISION "${PROJECT_VERSION_PATCH} % 100")

message("PROJECT_FQDN: ${PROJECT_FQDN}")
message("PROJECT_NAME: ${PROJECT_NAME}")
message("PROJECT_VERSION: ${PROJECT_VERSION}")
message("PROJECT_VERSION_MAJOR: ${PROJECT_VERSION_MAJOR}")
message("PROJECT_VERSION_MINOR: ${PROJECT_VERSION_MINOR}")
message("PROJECT_VERSION_PATCH: ${PROJECT_VERSION_PATCH}")
message("CMAKE_PROJECT_VERSION: ${CMAKE_PROJECT_VERSION}")
message("CMAKE_PROJECT_VERSION_MAJOR: ${CMAKE_PROJECT_VERSION_MAJOR}")
message("CMAKE_PROJECT_VERSION_MINOR: ${CMAKE_PROJECT_VERSION_MINOR}")
message("CMAKE_PROJECT_VERSION_PATCH: ${CMAKE_PROJECT_VERSION_PATCH}")
message("RC_VERSION_BUILD: ${RC_VERSION_BUILD}")
message("RC_VERSION_REVISION: ${RC_VERSION_REVISION}")
message("PROJECT_YEAR: ${PROJECT_YEAR}")
message("PROJECT_MONTH: ${PROJECT_MONTH}")
message("PROJECT_DAY: ${PROJECT_DAY}")

list(APPEND SUNSHINE_DEFINITIONS PROJECT_FQDN="${PROJECT_FQDN}")
list(APPEND SUNSHINE_DEFINITIONS PROJECT_NAME="${PROJECT_NAME}")
list(APPEND SUNSHINE_DEFINITIONS PROJECT_VERSION="${PROJECT_VERSION}")
list(APPEND SUNSHINE_DEFINITIONS PROJECT_VERSION_MAJOR="${PROJECT_VERSION_MAJOR}")
list(APPEND SUNSHINE_DEFINITIONS PROJECT_VERSION_MINOR="${PROJECT_VERSION_MINOR}")
list(APPEND SUNSHINE_DEFINITIONS PROJECT_VERSION_PATCH="${PROJECT_VERSION_PATCH}")
list(APPEND SUNSHINE_DEFINITIONS PROJECT_VERSION_COMMIT="${GITHUB_COMMIT}")

# ----------------------------------------------------------------------------
# SolarFlare fork identity.
# When SOLARFLARE_FORK is ON (the default on this repo), expose a compile-time
# macro so source code can identify itself as the fork. Off on upstream builds
# via -DSOLARFLARE_FORK=OFF so this file is safe to vendor back upstream.
# ----------------------------------------------------------------------------
option(SOLARFLARE_FORK "Build the SolarFlare fork branding (version banner + publisher URL)." ON)
if(SOLARFLARE_FORK)
    list(APPEND SUNSHINE_DEFINITIONS SOLARFLARE_FORK=1)
    list(APPEND SUNSHINE_DEFINITIONS SOLARFLARE_FORK_NAME="SolarFlare")
    list(APPEND SUNSHINE_DEFINITIONS SOLARFLARE_FORK_REPO="https://github.com/vindeckyy/Solar-Flare")
endif()
