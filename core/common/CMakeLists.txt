add_library(sage_common INTERFACE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_include_directories(sage_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sage_common
    INTERFACE
        cxx_std_20
)

target_compile_options(sage_common
    INTERFACE
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
)