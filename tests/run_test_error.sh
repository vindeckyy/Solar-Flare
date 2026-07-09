#!/usr/bin/env bash
# Standalone runner for src/error.h unit tests.
# ponytail: bypasses the cmake test target because the full test_sunshine
# build needs to compile every translation unit (boost::asio+ssl pulls
# gigabytes into the cc1plus process). This compiles only error.cpp +
# test_error.cpp and links against the static libs already on disk from
# the user's earlier cmake-build-test run. ~10s instead of OOM.
#
# Usage: tests/run_test_error.sh

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD="$ROOT/cmake-build-test"

if [ ! -d "$BUILD/_deps/boost-build/libs" ]; then
    echo "fatal: $BUILD/_deps/boost-build/libs not found. Run cmake once with" >&2
    echo "       -DBUILD_TESTS=ON to populate the boost build tree, then this" >&2
    echo "       script can compile and link the test in isolation." >&2
    exit 2
fi

BOOST_INCLUDES=$(find "$BUILD/_deps/boost-src/libs" -name include -type d | sed 's|^|-I|')
CXX="${CXX:-/usr/bin/c++}"

FLAGS=(
    -std=gnu++23
    -O0 -g
    -fno-pie -no-pie
    -I"$ROOT"
    $BOOST_INCLUDES
    -I"$BUILD/_deps/ffmpeg/include"
    -isystem "$ROOT/third-party"
    -isystem "$ROOT/third-party/googletest"
    -isystem "$ROOT/third-party/googletest/googletest/include"
    -isystem /usr/include/openssl
    -isystem /usr/include/libdrm
    -isystem /usr/include/libevdev-1.0
    -isystem /usr/include/pipewire-0.3
    -isystem /usr/include/spa-0.2
    -isystem /usr/include/gio-unix-2.0
    -isystem /usr/include/glib-2.0
    -isystem /usr/lib/glib-2.0/include
    -DBOOST_ATOMIC_NO_LIB
    -DBOOST_CHRONO_NO_LIB
    -DBOOST_DATE_TIME_NO_LIB
    -DBOOST_FILESYSTEM_NO_LIB
    '-DBOOST_FILESYSTEM_STATIC_LINK=1'
    -DBOOST_LOG_NO_LIB
    -DBOOST_PROCESS_STATIC_LINK
    -DBOOST_THREAD_USE_LIB
    -DSUNSHINE_TESTS
    -DSOLARFLARE_FORK=1
    '-DSUNSHINE_PLATFORM="linux"'
    '-DSUNSHINE_ASSETS_DIR="/tmp/dummy"'
    '-DSUNSHINE_TEST_BIN_DIR="/tmp/dummy"'
    '-DSUNSHINE_SHADERS_DIR="/tmp/dummy"'
    -DSUNSHINE_BUILD_VULKAN=1
    -DSUNSHINE_BUILD_DRM
    -DSUNSHINE_BUILD_X11
    -DSUNSHINE_BUILD_WAYLAND
    -DSUNSHINE_TRAY=1
    -DSUNSHINE_BUILD_HERMES_KMS
    -DSUNSHINE_BUILD_KWIN
    -DSUNSHINE_BUILD_PORTAL
    -DSUNSHINE_BUILD_VAAPI
    -Wall
    -Wno-sign-compare
    -fno-plt
)

LIBS=(
    "$BUILD/lib/libgtest.a"
    "$BUILD/_deps/boost-build/libs/log/libboost_log.a"
    "$BUILD/_deps/boost-build/libs/filesystem/libboost_filesystem.a"
    "$BUILD/_deps/boost-build/libs/thread/libboost_thread.a"
    "$BUILD/_deps/boost-build/libs/atomic/libboost_atomic.a"
    "$BUILD/_deps/boost-build/libs/chrono/libboost_chrono.a"
    "$BUILD/_deps/boost-build/libs/date_time/libboost_date_time.a"
    -lpthread
)

OUT="/tmp/test_error_runner"
echo "Compiling src/error.cpp + tests/unit/test_error.cpp + gtest_main..."
"$CXX" "${FLAGS[@]}" \
    "$ROOT/src/error.cpp" \
    "$ROOT/tests/unit/test_error.cpp" \
    "$ROOT/third-party/googletest/googletest/src/gtest_main.cc" \
    "${LIBS[@]}" \
    -o "$OUT" || { echo "build failed"; exit 1; }

echo "Running $OUT..."
"$OUT" --gtest_color=no
