param([switch]$ConfigureOnly)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mingw = 'C:\msys64\mingw64'
$env:PATH = "$mingw\bin;$env:PATH"
$env:MSYSTEM_PREFIX = $mingw.Replace('\', '/')
$hamlib = Join-Path $root '.tools\hamlib\hamlib-w64-4.7.2'
$build = Join-Path $root 'build-custom'
& "$mingw\bin\cmake.exe" -S $root -B $build -G Ninja `
    '-DCMAKE_BUILD_TYPE=Release' '-DCMAKE_POLICY_VERSION_MINIMUM=3.5' `
    "-DCMAKE_PREFIX_PATH=$mingw;$hamlib" "-DCMAKE_INSTALL_PREFIX=$root/dist-custom" `
    "-DOMNIRIG_TYPE_LIB=$root/.tools/OmniRig.tlb" `
    "-DCMAKE_LIBRARY_PATH=$hamlib/lib/gcc" `
    "-DDUMPCPP_Executable=$mingw/bin/dumpcpp-qt5.exe" `
    '-DWSJT_GENERATE_DOCS=OFF' '-DWSJT_SKIP_MANPAGES=ON' '-DWSJT_SKIP_MAP65=ON' `
    '-DWSJT_BUILD_UTILS=OFF' '-DWSJT_BUILD_TESTS=OFF' '-DWSJT_SKIP_BUNDLE_FIXUP=ON' `
    '-DCMAKE_Fortran_FLAGS=-fallow-argument-mismatch' '-DBoost_NO_BOOST_CMAKE=OFF' '-DCMAKE_POLICY_DEFAULT_CMP0167=NEW'
if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
if ($ConfigureOnly) { return }
& "$mingw\bin\cmake.exe" --build $build --target wsjtx jt9 wsprd fmtave fcal fmeasure wsjtx_app_version --parallel 6
if ($LASTEXITCODE) { throw 'Build failed.' }
