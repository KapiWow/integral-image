project(image_integrator)
set(SOURCE_LIB image_integrator.cc)

add_library(
    image_integrator STATIC 
    ${SOURCE_LIB}
)

target_link_libraries(
    image_integrator
    multithread_utils
    ${OpenCV_LIBS}
)

