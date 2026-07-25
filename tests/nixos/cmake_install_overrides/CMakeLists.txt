cmake_minimum_required(VERSION 3.20)

# Verify that caller-provided NixOS-safe installation directories take
# precedence over immutable paths reported by systemd.pc and udev.pc.
project(test_nixos_install_overrides NONE)

if(NOT DEFINED SOLARFLARE_SOURCE_ROOT)
    message(FATAL_ERROR "SOLARFLARE_SOURCE_ROOT is required")
endif()

list(PREPEND CMAKE_MODULE_PATH "${SOLARFLARE_SOURCE_ROOT}/cmake")

set(SYSTEMD_USER_UNIT_INSTALL_DIR "/home/test/.config/systemd/user")
set(SYSTEMD_SYSTEM_UNIT_INSTALL_DIR "/home/test/.local/lib/systemd/system")
set(SYSTEMD_MODULES_LOAD_DIR "/home/test/.local/lib/modules-load.d")
set(UDEV_RULES_INSTALL_DIR "/home/test/.local/lib/udev/rules.d")

find_package(Systemd REQUIRED)
find_package(Udev REQUIRED)

foreach(variable
        SYSTEMD_USER_UNIT_INSTALL_DIR
        SYSTEMD_SYSTEM_UNIT_INSTALL_DIR
        SYSTEMD_MODULES_LOAD_DIR
        UDEV_RULES_INSTALL_DIR)
    if("${${variable}}" MATCHES "^/nix/store/")
        message(FATAL_ERROR "${variable} was overwritten with ${${variable}}")
    endif()
endforeach()
