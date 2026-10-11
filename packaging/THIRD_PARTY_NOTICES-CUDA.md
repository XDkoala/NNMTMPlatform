# Windows CUDA package dependencies

NNMTMPlatform application source is MIT licensed. The MIT license does not
relicense the third-party binaries in this package.

- Qt 6.10.1 Core/Gui/Widgets and plugins: dynamically linked under LGPLv3.
  Complete notices, SBOMs and build configuration are in `licenses/Qt/`.
  ABI-compatible Qt DLL replacement and reverse engineering for debugging
  modifications to LGPL components are permitted. Qt Charts is not included.
  Corresponding qtbase/qtsvg sources are provided as `Qt-6.10.1-sources.zip`
  in the same Release.
- LibTorch 2.9.0 CUDA 12.8: BSD-style and third-party terms. Unmodified LICENSE
  and NOTICE from the matching official PyTorch 2.9.0+cu128 Windows wheel
  are in `licenses/PyTorch/` (including Intel OpenMP and embedded components).
- NVIDIA CUDA 12.8 runtime/math/compiler/profiling libraries and cuDNN 9.10.2:
  NVIDIA SDK license terms, not MIT. Complete CUDA EULA, CUPTI notices and
  cuDNN EULA (including its supplement) are
  in `licenses/NVIDIA/`. These unmodified redistributable runtime libraries
  are supplied only as components of this application, not as a standalone
  SDK. Their use and redistribution remain subject to NVIDIA's terms;
  do not redistribute them separately or remove NVIDIA proprietary notices.
  NVIDIA display drivers and CUDA developer tools are not included.
- OpenCV 4.11.0 and vcpkg dependencies: complete notices in `licenses/vcpkg/`.
- Microsoft Visual C++ runtime: Microsoft redistributable runtime files.
  Windows system libraries are not bundled.

License and corresponding-source locations:

- https://docs.nvidia.com/cuda/archive/12.8.0/eula/index.html
- https://docs.nvidia.com/deeplearning/cudnn/backend/v9.10.2/reference/eula.html
- https://docs.nvidia.com/cupti/12.8/copyright-and-licenses/index.html
- https://download.pytorch.org/whl/cu128/torch-2.9.0%2Bcu128-cp311-cp311-win_amd64.whl
- https://github.com/pytorch/pytorch/tree/v2.9.0
- https://github.com/opencv/opencv/tree/4.11.0
- https://download.qt.io/archive/qt/6.10/6.10.1/submodules/
- https://github.com/XDkoala/NNMTMPlatform/tree/v0.1.1

Application source is unchanged from v0.1.1. Later packaging scripts are in
the repository's main branch; manifest.json records the application source
commit and packaging commit separately.
