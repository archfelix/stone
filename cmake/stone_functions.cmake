function(stone_build_package)
    foreach(ARG IN LISTS ARGN)
        message(STATUS "build package: ${ARG}")
        add_subdirectory(src/${ARG})
    endforeach()
endfunction()