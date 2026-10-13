add_library(StuKitTasksDomain STATIC
    ITaskRepository.h
    TaskItem.cpp
    TaskItem.h
    TaskTypes.h
)

target_include_directories(StuKitTasksDomain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitTasksDomain
    PUBLIC
        Qt6::Core
)

target_compile_features(StuKitTasksDomain
    PUBLIC
        cxx_std_20
)
