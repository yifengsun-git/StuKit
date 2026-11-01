add_library(StuKitMiniz STATIC
    miniz.c
    miniz.h
)

set_source_files_properties(miniz.c PROPERTIES LANGUAGE CXX)

target_include_directories(StuKitMiniz
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(StuKitMiniz
    PUBLIC
        MINIZ_NO_STDIO
        MINIZ_NO_TIME
)

set_target_properties(StuKitMiniz PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(StuKitMiniz PRIVATE /W0)
else()
    target_compile_options(StuKitMiniz PRIVATE -w)
endif()
