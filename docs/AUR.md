# Arch and AUR

The reference [PKGBUILD](../packaging/arch/PKGBUILD) builds the tagged GitHub source archive, tests it with CTest, and installs through CMake. After the first approved GitHub release, run `updpkgsums`, `makepkg -si`, `makepkg --printsrcinfo > .SRCINFO`, and `namcap` against the package. Keep the AUR repository separate; only `PKGBUILD` and generated `.SRCINFO` belong there.

The AUR account has not yet been created, so no AUR upload should be attempted. Until then, clone the GitHub repository and use the CMake installation steps in the README.
