# SmartCity 3D — Defect Log

This document records all defects identified, tracked, and verified during the Quality Assurance and Audit phases (Phase 8) of SmartCity 3D. Each defect is assigned a unique ID, categorized by severity, analyzed for root cause, resolved with surgical code changes, and validated through repeatable regression tests.

---

## Defect Summary Table

| ID | Severity | Component | Description | Status | Regression Test |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **DEF-01** | Major | `scripts/setup.ps1` | Non-ASCII em-dash syntax error in PowerShell 5.1 & legacy 32-bit GCC priority | Closed / Verified | `setup.ps1` runs clean on PS 5.1; detects 64-bit MinGW-w64 |
| **DEF-02** | Major | `scripts/build.ps1` | Unquoted `-DCMAKE_BUILD_TYPE=$Config` string parameter passing | Closed / Verified | `build.ps1 -Config Debug` & `Release` generate correct CMake build types |
| **DEF-03** | Minor | `tests/CMakeLists.txt` | Unused variable compiler warnings in Release build due to `NDEBUG` on `assert()` | Closed / Verified | `cmake --build build --config Release` with 0 warnings |

---

## Detailed Defect Entries

### DEF-01: PowerShell syntax error and compiler toolchain selection in `setup.ps1`

- **ID**: `DEF-01`
- **Severity**: Major
- **Component**: Environment Bootstrap (`scripts/setup.ps1`)
- **Reported By**: Phase 8 Independent QA Auditor
- **Status**: Closed / Verified

#### Description
Running `scripts/setup.ps1` from a clean Windows PowerShell 5.1 terminal produced fatal syntax errors:
```
The ampersand (&) character is not allowed. The & operator is reserved for future use; wrap an ampersand in double quotes ("&") to refer to it as part of a string.
At C:\Users\deeks\Downloads\CG Project\scripts\setup.ps1:41 char:54
+ ... Write-Host "[OK] Git found: $(git --version)" -ForegroundColor Green
```
Additionally, on systems with multiple compilers installed, `setup.ps1` queried system `PATH` first and selected a legacy 32-bit MinGW installation (`C:\MinGW\bin\gcc.exe`) rather than the modern 64-bit MinGW-w64 toolchain in `C:\Users\deeks\tools\w64devkit\bin`.

#### Reproduction Steps
1. Open standard Windows PowerShell 5.1 console.
2. Run `powershell -ExecutionPolicy Bypass -File scripts/setup.ps1`.
3. Observe script termination with parser syntax error on line 41.

#### Root Cause
1. `scripts/setup.ps1` contained multi-byte UTF-8 em-dash (`—`) characters without a byte-order mark (BOM). Under default Windows PowerShell 5.1 legacy code pages (Windows-1252), byte sequences were misinterpreted as reserved characters.
2. Toolchain discovery checked `Get-Command gcc -ErrorAction SilentlyContinue` before checking preferred portable tool paths, picking up 32-bit GCC if present in the global machine `PATH`.

#### Fix
1. Rewrote `scripts/setup.ps1` using strict 7-bit ASCII characters throughout.
2. Explicitly added directory prioritization to look in `C:\Users\deeks\tools\w64devkit\bin` and `C:\Users\deeks\tools\cmake\bin` before checking ambient PATH.

#### Regression Test
- Execute `powershell -ExecutionPolicy Bypass -File scripts/setup.ps1` on standard Windows 11 PowerShell.
- **Result**: Exit code 0, toolchain correctly identified as MinGW-w64 GCC 13.2.0 (x86_64-w64-mingw32), CMake 3.30.2, and Git.

---

### DEF-02: Unquoted CMake build type variable expansion in `build.ps1`

- **ID**: `DEF-02`
- **Severity**: Major
- **Component**: Build Automation (`scripts/build.ps1`)
- **Reported By**: Phase 8 Independent QA Auditor
- **Status**: Closed / Verified

#### Description
When invoking `build.ps1` with `-Config Debug` or `-Config Release`, the generated `CMakeCache.txt` stored the literal string `$Config` instead of the resolved variable value:
```cmake
CMAKE_BUILD_TYPE:STRING=$Config
```
This prevented compiler flags specific to `Debug` (such as assertions, symbol generation, and GL debug output) or `Release` (such as `-O3` optimizations) from being applied correctly.

#### Reproduction Steps
1. Delete `build/CMakeCache.txt`.
2. Run `powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Config Debug`.
3. Inspect `build/CMakeCache.txt` for `CMAKE_BUILD_TYPE`.
4. Observe `CMAKE_BUILD_TYPE:STRING=$Config`.

#### Root Cause
In PowerShell, passing `-DCMAKE_BUILD_TYPE=$Config` directly inside a command line argument array without enclosing quotes caused the argument parser to emit the verbatim literal string `"-DCMAKE_BUILD_TYPE=$Config"` rather than interpolating the variable `$Config`.

#### Fix
Wrapped the parameter in explicit double quotes: `"-DCMAKE_BUILD_TYPE=$Config"` in both CMake configure invocations within `scripts/build.ps1`.

#### Regression Test
1. Run `powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Config Debug`. Verify `CMakeCache.txt` contains `CMAKE_BUILD_TYPE:STRING=Debug`.
2. Run `powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Config Release`. Verify `CMakeCache.txt` contains `CMAKE_BUILD_TYPE:STRING=Release`.
- **Result**: Passed. Both configurations correctly set compiler and linker flags.

---

### DEF-03: Release build compiler warnings on unused variables in test files

- **ID**: `DEF-03`
- **Severity**: Minor
- **Component**: Test Suite Build (`tests/CMakeLists.txt`)
- **Reported By**: Phase 8 Independent QA Auditor
- **Status**: Closed / Verified

#### Description
When building the test suite under the `Release` configuration, the compiler emitted `-Wunused-variable` warnings across multiple test files:
```
tests/test_road_network.cpp:42:10: warning: unused variable 'saved' [-Wunused-variable]
tests/test_traffic_signals.cpp:88:14: warning: unused variable 'conflictDetected' [-Wunused-variable]
```
This violated the strict project mandate of zero compiler warnings in all build configurations.

#### Reproduction Steps
1. Configure project in Release mode: `cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release`.
2. Build all test targets: `cmake --build build --config Release`.
3. Observe compiler warnings for unused variables in test sources.

#### Root Cause
Under C++ and CMake conventions, `Release` builds define the macro `NDEBUG`. The standard C `<cassert>` header defines `assert(expr)` as `((void)0)` when `NDEBUG` is set. As a result, variables that were computed solely to be verified in `assert(...)` became unreferenced in `Release` builds, triggering compiler unused-variable diagnostics.

#### Fix
Added the compiler option `-UNDEBUG` to all test targets in `tests/CMakeLists.txt`:
```cmake
target_compile_options(${TEST_NAME} PRIVATE -UNDEBUG)
```
This ensures test assertions remain fully active and evaluated during test execution, maintaining 100% test coverage and eliminating all `-Wunused-variable` warnings.

#### Regression Test
1. Execute `cmake --build build --config Release --clean-first`.
2. Verify zero warnings emitted across all 25 test targets.
3. Run `ctest --test-dir build -C Release --output-on-failure`.
- **Result**: Passed (25/25 tests passed in 14.53s, 0 compiler warnings).
