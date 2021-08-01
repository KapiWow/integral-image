project(task_pool)
set(SOURCE_LIB task_pool.cc)

add_library(
    task_pool STATIC 
    ${SOURCE_LIB}
)

target_link_libraries(
    task_pool
    pthread
)

