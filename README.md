# NFS-RainController

This repository started as a disassembly of the **NFS-RainController** plugin.
The code base now includes a partial C++ reimplementation.  Several
classes have been stubbed out to mirror the original plugin's feature
structure.  The Visual Studio 2017 (VC++ 17) solution under `vs17`
builds the C++ sources into a dynamic library.

### Features

The following feature classes are declared:

- `ForcePrecipitation` – standalone implementation of the
  precipitation logic inspired by NFSC 1.4

### Dependencies

The original binary links against `d3dx9_43.dll` and `fmt.dll`. The
project therefore expects the DirectX SDK and fmt library to be
available. Set the following environment variables so the build system
can locate the headers and libraries:

```
set DXSDK_DIR=C:\path\to\DirectXSDK\
set FMT_DIR=C:\path\to\fmt\
```

The Visual Studio project uses these variables for include and library
directories and links against `d3d9.lib`, `d3dx9.lib`, `fmt.lib`, and
`ws2_32.lib`.

Open `NFS-RainController.sln` with Visual Studio 2017 or later and build the solution.

### Address calculations

The assembly listings located in the `disasm/` directory were produced with IDA
and show addresses relative to the plugin image base.  When patching bytes at
runtime these static addresses must be translated to the actual load address of
the module or the game executable.  `GameAddresses.h` provides helper functions
and macros for this purpose:

```
// Convert an address from the game executable
auto ptr = MW_EXE_PTR(BYTE, 0x006DBE79);
// Convert an address within the plugin itself
auto flag = MW_MODULE_PTR(BYTE, 0x10112277);
```

To locate new offsets in a compiled `.asi`:
1. Open the binary in IDA and note the image base printed near the top of the
   disassembly.
2. Find the instruction or data you are interested in and record its address.
3. Subtract the static base (`0x400000` for the game or `0x10000000` for the
   plugin) to obtain an offset.
4. At runtime add that offset to the module's load address obtained via
   `GetModuleHandleA(NULL)` (game) or `GetModuleHandleExA` (plugin).

This mirrors the approach used in the VerbleHackNFSMW project which stores
absolute addresses in the source and adjusts them at runtime.

Additional reference listings under `disasm/` document how precipitation
logic works in NFSC. These files can help port the rain feature to other
games that lack native support.
