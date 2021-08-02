add_subdirectory(multithread_utils)
add_subdirectory(image_integrator)
add_subdirectory(tests)

set(
    SOURCE_EXE 
    main.cc
)

add_executable(
    integrate_image 
    ${SOURCE_EXE}
)

target_link_libraries(
    integrate_image 
    image_integrator
)
