add_library(StuKitTasksPresentation STATIC
    TasksViewModel.cpp
    TasksViewModel.h
)

target_include_directories(StuKitTasksPresentation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StuKitTasksPresentation PUBLIC Qt6::Core StuKitTasksApplication)
target_compile_features(StuKitTasksPresentation PUBLIC cxx_std_20)
