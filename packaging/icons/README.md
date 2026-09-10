# Red Alert application icon

The icon is the original 32×32 game icon from group 1 of the local
`assets/redalert/soviet/INSTALL/RA95.EXE` Windows resources. It retains the
original game's artwork and ownership; it is not newly licensed artwork.

`redalert.ico` preserves the original resource. `redalert.png` is a 256×256
nearest-neighbor enlargement for Linux launchers. `redalert.icns` contains
macOS icon sizes through 1024×1024, also enlarged without smoothing.
`PORT/MAC/include/ra_window_icon.h` contains the original 32×32 RGBA pixels
for SDL windows, so the runtime needs no external icon file or image library.

The Mac playtest packager installs the ICNS into `Contents/Resources` and
declares it in `Info.plist`. Linux CMake builds generate `redalert.desktop`
in the build directory, pointing to that checkout and build directory.
Copy it to `~/.local/share/applications/` to add the game to the applications
menu. Regenerate and recopy it if the checkout or build directory moves.
