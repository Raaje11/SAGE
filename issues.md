# Project Issues Log

## Build & Configuration Issues

### ❌ CMake Compiler Test Failure
- **File:** `C:\Program Files\CMake\share\cmake-4.2\Modules\CMakeTestCCompiler.cmake` (Line 67)
- **Severity:** `Error`
- **Description:** The C compiler (`clang.exe`) is unable to compile a simple test program.
- **Details:** The linker `lld-link` is failing to locate standard Windows system libraries.
- **Missing Libraries:**
  - `kernel32.lib`, `user32.lib`, `gdi32.lib`, `winspool.lib`, `shell32.lib`, `ole32.lib`, `oleaut32.lib`, `uuid.lib`, `comdlg32.lib`, `advapi32.lib`
- **Possible Fix:** Ensure the Windows SDK is installed and its `lib` paths are added to the environment variables (`LIB` or `PATH`), or run the build from a "Developer Command Prompt for VS".

---

## Code Standard Warnings

### ⚠️ C++17 Extension Usage
- **Locations:**
  - `e:\SAGE\core\common\Constants.hpp` (Line 10)
  - `e:\SAGE\core\common\Types.hpp` (Line 10)
- **Severity:** `Warning`
- **Message:** `Nested namespace definition is a C++17 extension; define each namespace separately (fix available)`
- **Description:** The code uses `namespace A::B { ... }` which is only officially supported from C++17 onwards. 
- **Possible Fix:** 
  - Update `CMakeLists.txt` to enforce C++17 or higher: `set(CMAKE_CXX_STANDARD 17)`
  - Or revert to separate definitions:
    ```cpp
    namespace A {
        namespace B {
            // ...
        }
    }
    ```
