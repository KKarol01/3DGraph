add_executable(opengl_engine "main.cpp"
"3rdparty/glad.c"
"3rdparty/include/imgui/imgui.cpp"
"3rdparty/include/imgui/imgui_draw.cpp"
"3rdparty/include/imgui/imgui_impl_glfw.cpp"
"3rdparty/include/imgui/imgui_impl_opengl3.cpp"
"3rdparty/include/imgui/imgui_stdlib.cpp"
"3rdparty/include/imgui/imgui_tables.cpp"
"3rdparty/include/imgui/imgui_widgets.cpp"
"orbital_camera.cpp"
"renderer.cpp"
)

set_property(TARGET opengl_engine PROPERTY CXX_STANDARD 20)

target_link_directories(opengl_engine PRIVATE "3rdparty/lib")
target_include_directories(opengl_engine PRIVATE "3rdparty/include" ".")
target_link_libraries(opengl_engine PRIVATE "glfw3" "nfd")

file(COPY "fonts" DESTINATION ".")

#target_compile_definitions(opengl_engine PRIVATE )
add_subdirectory("3rdparty")
#add_subdirectory("assets")