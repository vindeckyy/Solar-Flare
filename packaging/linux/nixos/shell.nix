## @file
# @brief Reproducible NixOS build environment for SolarFlare.
#
# This shell supplies the compiler, Web UI tooling, and native libraries used
# by scripts/cachyos-build.sh. Runtime graphics drivers remain supplied by the
# host NixOS configuration.
{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  packages = with pkgs; [
    avahi
    boost
    cmake
    curl
    git
    glib
    glslang
    libcap
    libdrm
    libevdev
    libgbm
    libpng
    libportal
    libpulseaudio
    libva
    libx11
    libxcb
    libxext
    libxfixes
    libxkbcommon
    libxrandr
    libxtst
    miniupnpc
    ninja
    nlohmann_json
    nodejs
    openssl
    opus
    pipewire
    pkg-config
    shaderc
    spirv-tools
    systemd
    vulkan-headers
    vulkan-loader
    wayland
    wayland-protocols
    wayland-scanner
    (python3.withPackages (pythonPackages: with pythonPackages; [
      jinja2
      setuptools
    ]))
  ];
}
