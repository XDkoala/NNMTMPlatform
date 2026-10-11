# Windows CPU package dependencies

NNMTMPlatform source code is MIT licensed. Bundled third-party components are
covered by their own licenses, not the project's MIT license.

- Qt 6.10.1 (Qt Core, Gui, Widgets and deployed plugins): dynamically linked
  under LGPLv3. Qt Charts is not included. License text, copyright information
  and supplier SBOMs are included in `licenses/Qt/`.
- LibTorch 2.9.0 CPU: BSD-style license and third-party licenses. The matching
  official PyTorch CPU wheel's consolidated LICENSE and NOTICE are included
  in `licenses/PyTorch/` (including Intel OpenMP and bundled CPU components).
- OpenCV 4.11.0 and its vcpkg runtime dependencies: each component's copyright
  and license files are included in `licenses/vcpkg/`.
- Microsoft Visual C++ runtime: Microsoft's redistributable runtime files;
  Windows system libraries are not bundled.

The Qt DLLs can be replaced with ABI-compatible builds. No restriction is
placed on reverse engineering for debugging modifications to LGPL components.
This package uses unmodified official Qt 6.10.1 runtime libraries.

Corresponding Qt sources are supplied as `Qt-6.10.1-sources.zip` alongside the
Windows download in the same GitHub Release. The ZIP contains qtbase and qtsvg
source archives. The build configuration files are in `licenses/Qt/`.

Upstream sources:

- https://download.qt.io/archive/qt/6.10/6.10.1/submodules/
- https://github.com/pytorch/pytorch/tree/v2.9.0
- https://github.com/opencv/opencv/tree/4.11.0

The corresponding NNMTMPlatform source and build scripts are available from
the source archive for this Release's tag at:
https://github.com/XDkoala/NNMTMPlatform/releases
