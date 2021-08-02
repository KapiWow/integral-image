project(multithread_utils)
set(SOURCE_LIB thread_pool.cc)

add_library(
    multithread_utils STATIC 
    ${SOURCE_LIB}
)

target_link_libraries(
    multithread_utils
    pthread
)

