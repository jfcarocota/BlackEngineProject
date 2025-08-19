# BlackEngine Project - Runtime Dependencies

## Windows Deployment Error Fix

If you get the error "MSVCP140.dll was not found" when running TileMapEditor.exe on another PC:

### Quick Fix - Install Visual C++ Redistributables

Download and install **Microsoft Visual C++ Redistributable** for your architecture:

- **x64**: https://aka.ms/vs/17/release/vc_redist.x64.exe
- **x86**: https://aka.ms/vs/17/release/vc_redist.x86.exe

### Alternative - Portable Deployment

Copy these files from your development PC to the same folder as TileMapEditor.exe:

```
# From your Windows system (usually C:\Windows\System32\):
msvcp140.dll
vcruntime140.dll
vcruntime140_1.dll (if exists)

# From SFML build directory (if not already copied by CMake):
sfml-*.dll files
```

### Automated Solution (Recommended)

The CMakeLists.txt is configured to copy runtime DLLs automatically. To ensure this works:

1. Build in Debug or Release mode
2. Check that DLLs are copied to `build/bin/Debug/` or `build/bin/Release/`
3. Distribute the entire folder contents, not just the .exe

### Creating a Redistributable Package

Run this command to create a portable ZIP:

```powershell
# Build and package
cmake --build build --config Release
cmake --build build --target package --config Release
```

This creates a ZIP file in `build/` with all necessary runtime dependencies.

### Verification

Test your deployment by:
1. Copying the build output to a clean folder
2. Running on a PC without Visual Studio installed
3. Checking that all .dll files are present alongside the .exe
