# Build and development

[Deutsch](Build-und-Entwicklung) | [English](Build-and-development)

## One build, one program package

| Directory | Purpose |
| --- | --- |
| `build-custom-make` | Single local Custom build using MinGW Makefiles, with tests in subdirectories. |
| `dist-custom` | Shared program package for all launchers. |
| `.tools` | Local SDKs and runtime dependencies. |
| `.local` | Private configurations, archives and download artifacts. |

Work in the visible main project directory. A second Custom build or multiple program copies for different radio profiles are unnecessary.

## Build on Windows

The tools default to MSYS2/MinGW64 at `C:\msys64\mingw64`: GCC/G++, Fortran, CMake, MinGW Make, Qt5, Boost, FFTW and libusb. `WSJTX_MINGW_PREFIX` can override the path. The preparation script downloads pinned Hamlib/OmniRig dependencies and verifies their checksums.

```powershell
.\tools\Setup-CustomBuild.ps1
.\tools\Build-Custom.ps1
.\tools\Test-Core.ps1
.\tools\Test-PskReporter.ps1
.\tools\Test-WebControl.ps1
.\tools\Test-Reist16.ps1
.\tools\Test-TxQuality.ps1
.\tools\Test-Ft2.ps1
.\tools\Test-PlutoBridge.ps1
.\tools\Test-HackrfBridge.ps1
.\tools\Package-Custom.ps1
.\tools\Verify-CustomPackage.ps1 -PortableOnly
```

To rebuild and update the shared package:

```powershell
.\tools\Rebuild-AllCustom.ps1
# For an incremental build:
.\tools\Rebuild-AllCustom.ps1 -Incremental
```

The name `Rebuild-AllCustom.ps1` is retained for compatibility; it does not recreate the old variant directories. MAP65 and additional simulators are not part of the Custom Windows package.

## Upstream and releases

Official `WSJTX/wsjtx:master` history was reconciled on 26 October 2026. The 103 previously unmatched commit identities already had equivalent patches locally; the history merge did not change the Custom source tree. [Merge and explanation](https://github.com/rudolfstepan/wsjtx/commit/ae17a3bbc9d0679e2bd806466ce7ed53313ba48c).

GitHub Actions is deliberately disabled at present. **RC3 was built and validated locally.** The existing workflow can build Windows packages if re-enabled; it does not establish a current successful CI run. RC3 was manually designated Latest.

## Maintain the wiki

Editable sources live in the main repository under `docs/wiki`. After committing content changes, publish them with `python tools/Publish-Wiki.py`. The publisher preserves unmanaged existing wiki pages and requires neither a second source checkout nor a Git worktree. Each topic has German and English editions with direct language links; `--check` verifies counterparts and links before publication.

[Build details, German](https://github.com/rudolfstepan/wsjtx/blob/master/CUSTOM-BUILD.md) · [Licence](https://github.com/rudolfstepan/wsjtx/blob/master/COPYING) · [Trademark policy](https://github.com/rudolfstepan/wsjtx/blob/master/TRADEMARK.md)
