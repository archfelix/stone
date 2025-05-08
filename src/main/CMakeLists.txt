add_executable(main main.cpp)
target_link_libraries(main stone)
target_link_libraries(main example_pub)
target_link_libraries(main example_sub)