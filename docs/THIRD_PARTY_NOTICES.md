# Third-party notices

## miniz 3.1.2

StuKit vendors `miniz` 3.1.2 for reading and validating ZIP-based theme packages.

- Project: <https://github.com/richgel999/miniz>
- Release archive SHA-256:
  `F0446D863F9C19926AD9483C523FDC42E42B8D4A6A431D27E09D49C79A140D9A`
- License: MIT (`src/third_party/miniz/LICENSE`)

The dependency is wrapped by `ThemePackageReader`; application and QML code should not call the
miniz API directly.
