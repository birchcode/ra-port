# Red Alert application icon

The icon is based on the original 32×32 game icon from group 1 of the local
`assets/redalert/soviet/INSTALL/RA95.EXE` Windows resources. It retains the
original game's artwork and ownership; it is not newly licensed artwork.

`redalert.ico` preserves the original resource as a reference.
`redalert-master.png` is an AI-assisted high-resolution remaster of that icon,
preserving the gold emblem, red palette, and diagonal background accents.
`redalert-terminal-master.png` adds an aged Soviet computer bezel and CRT glass
around that emblem and is the current export source.
`redalert.png` is the 512×512 Linux launcher export of the transparent
`redalert-macos.png`, preserving its alpha and margins. `redalert.icns` contains
macOS icon sizes through 1024×1024. Its source, `redalert-macos.png`, uses
an 824×824 industrial rounded square on a transparent 1024×1024 canvas
(100-pixel margins), keeping the frame's corner details visible.
Traditional ICNS bundles need to supply their own shape and padding.
Regenerate this Mac source without altering the artwork using
the compile and run commands in `scripts/render_macos_icon.m`.
ICNS size exports use Lanczos resampling.
`PORT/MAC/include/ra_window_icon.h` contains a transparent 128×128 RGBA export
of the same padded image for SDL
windows, so the runtime needs no external icon file or image library.

The Mac playtest packager installs the ICNS into `Contents/Resources` and
declares it in `Info.plist`. Linux CMake builds generate `redalert.desktop`
in the build directory, pointing to that checkout and build directory.
Copy it to `~/.local/share/applications/` to add the game to the applications
menu. Regenerate and recopy it if the checkout or build directory moves.
