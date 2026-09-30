# Release process

1. Update the CMake version, changelog, metainfo release and PKGBUILD version.
2. Build, run CTest, validate desktop metadata and make a staged install.
3. Commit and tag `vX.Y.Z`; push only after approval.
4. Replace `SKIP` in the PKGBUILD with the actual SHA-256 of the GitHub tag archive using `updpkgsums`.
5. Build with `makepkg`, inspect with `namcap`, and publish the GitHub release.
6. When an AUR account is available, copy PKGBUILD and generated `.SRCINFO` to the separate AUR repository.

The GitHub source archive cannot have a final checksum until its tag exists. The upstream PKGBUILD therefore contains `SKIP` before publication; change it before an AUR upload.
