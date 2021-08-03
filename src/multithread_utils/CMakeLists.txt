project(multithread_utils)

set(
    SOURCE_LIB 
    thread_pool.cc
    log.cc
)

add_library(
    multithread_utils STATIC 
    ${SOURCE_LIB}
)

IF (UNIX)
    target_link_libraries(
        multithread_utils
        pthread
    )
ENDIF()
