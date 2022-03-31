add_executable(
    rsafefs
    rsafefs.cpp
)

target_link_libraries(
    rsafefs
    remote-safefs
    fmt
    CLI11
)