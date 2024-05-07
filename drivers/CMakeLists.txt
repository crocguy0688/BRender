add_subdirectory(glrend)

if (TARGET SDL2::SDL2)
    add_subdirectory(sdl2dev)
    target_link_libraries(brender-full INTERFACE sdl2dev)
else ()
    message("SDL2 not found, not building sdl2dev")
endif ()

target_link_libraries(brender-full INTERFACE glrend)

add_subdirectory(softrend)
target_link_libraries(brender-full INTERFACE softrend)

if (BRENDER_BUILD_SOFT)
    add_subdirectory(pentprim)
    target_link_libraries(brender-full INTERFACE softrend pentprim)
endif ()
