#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_stdlib.h>

#include <orbital_camera.hpp>
#include <renderer.hpp>

struct AppState {
    g3d::Window window;
} app_state;

// OrbitalCamera *orb_cam;
// float imgui_mouse_wheel{0.f};
// static void draw_settings_panel();

// template <typename T, typename DRAW_CB> static void display_2col_list(std::vector<T> &data, DRAW_CB draw_callback);

static void start_application(const char* window_title, uint32_t window_width, uint32_t window_height);
static void terminate_application();

// static void save_project(const char *file_name);
// static void load_project(const char *file_name);

// static glm::vec4 background_color{0.1};
// struct {
//     unsigned detail;
//     unsigned bounds;
// } plane_setttings;

// struct Function {
//     std::string name, value;
// };
// struct Slider {
//     Slider() = default;
//     Slider(const std::string &name, float value) : name{name}, value{value} {}

//     std::string name;
//     float value{0.f}, min{0.f}, max{1.0f};
// };
// struct Constant {
//     std::string name;
//     float value{0.f};
// };

// std::vector<Function> functions;
// std::vector<Slider> sliders;
// std::vector<Constant> constants;

int main() {
    start_application("3DCalc", 1280, 960);

    g3d::HandleFramebuffer framebuffer_main;
    g3d::HandleTexture texture_fmain_color;
    g3d::HandleTexture texture_fmain_depth_stencil;

    g3d::HandleProgram program_plane;
    g3d::HandleProgram program_line;
    g3d::HandleShader shader_plane_vert;
    g3d::HandleShader shader_plane_frag;
    g3d::HandleShader shader_line_vert;
    g3d::HandleShader shader_line_frag;

    g3d::HandleVao vao_plane;
    g3d::HandleVao vao_line;
    g3d::HandleBuffer vbo_plane, ebo_plane;
    g3d::HandleBuffer vbo_line;

    glCreateFramebuffers(1, &framebuffer_main);
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_fmain_color);
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_fmain_depth_stencil);
    glTextureStorage2D(texture_fmain_color, 1, GL_RGB8, app_state.window.width, app_state.window.height);
    glTextureStorage2D(texture_fmain_depth_stencil, 1, GL_DEPTH24_STENCIL8, app_state.window.width, app_state.window.height);
    glNamedFramebufferTexture(framebuffer_main, GL_COLOR_ATTACHMENT0, texture_fmain_color, 0);
    glNamedFramebufferTexture(framebuffer_main, GL_DEPTH_STENCIL_ATTACHMENT, texture_fmain_depth_stencil, 0);
    assert((glCheckNamedFramebufferStatus(framebuffer_main, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) && "Main framebuffer initialisation error.");

    program_plane = glCreateProgram();
    shader_plane_vert = glCreateShader(GL_VERTEX_SHADER);
    shader_plane_frag = glCreateShader(GL_FRAGMENT_SHADER);

    const char* shader_plane_vert_src = R"glsl(
        #version 460 core
        layout(location=0) in vec2 position;

        void main() {gl_Position = vec4(position, 0.0, 1.0); }
    )glsl";

    const char* shader_plane_frag_src = R"glsl(
        #version 460 core
        out vec4 FRAG_COLOR;

        void main() { FRAG_COLOR = vec4(0.3, 1.0.xxx); }
    )glsl";

    glShaderSource(shader_plane_vert, 1, &shader_plane_vert_src, 0);
    glShaderSource(shader_plane_frag, 1, &shader_plane_frag_src, 0);
    glCompileShader(shader_plane_vert);
    glCompileShader(shader_plane_frag);
    glAttachShader(program_plane, shader_plane_vert);
    glAttachShader(program_plane, shader_plane_frag);
    glLinkProgram(program_plane);
    {
        int link_status = 0;
        glGetProgramiv(program_plane, GL_LINK_STATUS, &link_status);
        assert((link_status == GL_TRUE) && "Plane shader program could not be linked properly."); 
    }

    glCreateVertexArrays(1, &vao_plane);
    glCreateBuffers(1, &vbo_plane);
    glCreateBuffers(1, &ebo_plane);
    glVertexArrayVertexBuffer(vao_plane, 0, vbo_plane, 0, 8);
    glVertexArrayElementBuffer(vao_plane, ebo_plane);
    glEnableVertexArrayAttrib(vao_plane, 0);
    glVertexArrayAttribBinding(vao_plane, 0, 0);
    glVertexArrayAttribFormat(vao_plane, 0, 2, GL_FLOAT, GL_FALSE, 0);

    glCreateVertexArrays(1, &vao_line);
    glCreateBuffers(1, &vbo_line);
    glVertexArrayVertexBuffer(vao_line, 0, vbo_line, 0, 12);
    glEnableVertexArrayAttrib(vao_line, 0);
    glVertexArrayAttribBinding(vao_line, 0, 0);
    glVertexArrayAttribFormat(vao_line, 0, 3, GL_FLOAT, GL_FALSE, 0);

    float triangle[]{
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };
    unsigned ebo[]{0,1,2};

    glNamedBufferStorage(vbo_plane, 24, triangle, 0);
    glNamedBufferStorage(ebo_plane, 12, ebo, 0);

    auto& window = app_state.window;
    while(glfwWindowShouldClose(window.pglfw_window) == false) {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(vao_plane);
        glUseProgram(program_plane);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window.pglfw_window);
    }
//     Engine::initialise("3DGraph", 1920, 1080);
//     auto &eng        = Engine::instance();
//     auto &gpu        = *eng.get_gpu_res_mgr();
//     auto &win        = *eng.get_window();
//     auto &gui        = *eng.get_gui();
//     orb_cam          = new OrbitalCamera{&win, OrbitalCameraSettings{90.0f, 0.1f, 100.0f}};
//     auto *buffer_vbo = gpu.create_resource(GLBuffer{GL_DYNAMIC_STORAGE_BIT});
//     auto *buffer_ebo = gpu.create_resource(GLBuffer{GL_DYNAMIC_STORAGE_BIT});
//     auto *line_vbo   = gpu.create_resource(GLBuffer{GL_DYNAMIC_STORAGE_BIT});
//     GLVao *line_vao, *vao;
//     ShaderProgram *line_shader = gpu.create_resource(ShaderProgram{"default/axis"});
//     // clang-format off
//     {
//     float vbo[] {
//         -1.0,  1.0,
//          1.0,  1.0,
//         -1.0, -1.0,
//          1.0, -1.0
//     };
//     unsigned ebo[]{0, 1, 2, 2, 1, 3};
//         buffer_vbo->push_data(vbo, sizeof(vbo));
//         buffer_ebo->push_data(ebo, sizeof(ebo));
//         vao = gpu.create_resource(GLVao{{GLVaoBinding{0, buffer_vbo->res_handle(), 8, 0}},
//                                               {GLVaoAttribute{0, 0, 2, 0}},
//                                               buffer_ebo->res_handle()});

//         float lineverts[]{
//             -1.0, 0.0, 0.0, 1.0, 0.0, 0.0,
//              0.0,-1.0, 0.0, 0.0, 1.0, 0.0,
//              0.0, 0.0,-1.0, 0.0, 0.0, 1.0,
//         };
//         line_vbo->push_data(lineverts, sizeof(lineverts));
//         line_vao = gpu.create_resource(GLVao{
//                     {GLVaoBinding{0, line_vbo->res_handle(), 12, 0}},
//                     {GLVaoAttribute{0, 0, 3, 0}} });
//         // clang-format on
//     }

//     ShaderProgram *shader = nullptr;

//     auto *texture_render
//         = gpu.create_resource(Texture{TextureSettings{}, TextureDataDescriptor{1920, 1080}});
//     auto *texture_depth = gpu.create_resource(
//         Texture{TextureSettings{GL_DEPTH24_STENCIL8, GL_CLAMP_TO_EDGE, GL_LINEAR, 1},
//                 TextureDataDescriptor{1920, 1080}});

//     FramebufferAttachment fbo_attachments[]{
//         FramebufferAttachment{GL_COLOR_ATTACHMENT0, texture_render->res_handle()},
//         FramebufferAttachment{GL_DEPTH_STENCIL_ATTACHMENT, texture_depth->res_handle()}};
//     auto *fbo = gpu.create_resource(Framebuffer{{fbo_attachments[0], fbo_attachments[1]}});

//     ImVec2 canvas_size, image_size;
//     auto resize_fbo = [&](auto w, auto h) {
//         texture_render = gpu.create_resource(
//             Texture{TextureSettings{}, TextureDataDescriptor{(int)w, (int)h}});
//         texture_depth = gpu.create_resource(
//             Texture{TextureSettings{GL_DEPTH24_STENCIL8, GL_CLAMP_TO_EDGE, GL_LINEAR, 1},
//                     TextureDataDescriptor{(int)w, (int)h}});

//         fbo_attachments[0]
//             = FramebufferAttachment{GL_COLOR_ATTACHMENT0, texture_render->res_handle()};
//         fbo_attachments[1]
//             = FramebufferAttachment{GL_DEPTH_STENCIL_ATTACHMENT, texture_depth->res_handle()};

//         fbo->update_attachments({fbo_attachments[0], fbo_attachments[1]});
//     };

//     bool needs_a_resize      = true;
//     bool needs_recompilation = true;
//     win.on_resize.connect([&](auto, auto) { needs_a_resize = true; });

//     functions.push_back({"f", "0.0"});

//     auto recompile_shader = [&] {
//         std::cout << "Recompiling\n";

//         std::string forward_declarations;
//         std::string function_definitions;
//         std::string uniforms;
//         std::string consts;

//         for (const auto &f : functions) {
//             forward_declarations += "float " + f.name + "(float,float);\n";
//             function_definitions += "float " + f.name + "(float x,float z){return ";
//             function_definitions += f.value + ";}\n";
//         }

//         for (const auto &c : constants) {
//             consts += "const float " + c.name + "=" + std::to_string(c.value) + ";\n";
//         }

//         for (const auto &s : sliders) { uniforms += "uniform float " + s.name + ";\n"; }

//         std::string shader_source;
//         shader_source += "#version 460 core\n"
//                          "layout(location=0)in vec2 in_pos;\n"
//                          "uniform mat4 p;\n"
//                          "uniform mat4 v;\n"
//                          "uniform mat4 m;\n"
//                          "uniform float TIME;\n"
//                          "uniform float detail;\n"
//                          "uniform float size;\n";
//         shader_source += uniforms;
//         shader_source += forward_declarations;
//         shader_source += consts;
//         shader_source += R"glsl(
// void main() {
//     vec2 ip = in_pos * 0.5+0.5;
//     float inv_detail = size/detail;
//     ip = ip * inv_detail - size*0.5;
//     ip.x += float(gl_InstanceID % uint(detail)) * inv_detail;
//     ip.y += float(gl_InstanceID / uint(detail)) * inv_detail;
//     float x = ip.x;
//     float y = ip.y;

//     gl_Position = p * v * vec4(x, f(x,y), y, 1.0);
// }
// )glsl";
//         shader_source += function_definitions;

//         std::string frag_src = R"glsl(
//             #version 460 core
//             out vec4 FRAG_COLOR;
//             void main() { FRAG_COLOR = vec4(1.0); }
//         )glsl";

//         try {
//             if (shader == nullptr) {
//                 shader = gpu.create_resource(ShaderProgram{
//                     {{ShaderType::Vertex, shader_source}, {ShaderType::Fragment, frag_src}}});
//             }
//             *shader = ShaderProgram{
//                 {{ShaderType::Vertex, shader_source}, {ShaderType::Fragment, frag_src}}};
//         } catch (const std::exception &e) { std::cout << e.what(); }
//     };
//     gui.add_draw([&]() {
//         auto main_screen_flags
//             = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar;

//         ImGui::SetNextWindowPos(ImVec2(0, 0));
//         ImGui::SetNextWindowSize(ImVec2(win.width(), win.height()));
//         if (ImGui::Begin("main screen", 0, main_screen_flags)) {
//             if (ImGui::BeginMenuBar()) {
//                 if (ImGui::BeginMenu("Project...")) {
//                     if (ImGui::Button("Open project")) {
//                         nfdchar_t *outPath = NULL;
//                         nfdresult_t result = NFD_OpenDialog("3dg", std::filesystem::current_path().string().c_str(), &outPath);
//                         if (result == NFD_OKAY) {
//                             load_project(outPath);
//                             needs_recompilation = true;
//                         }
//                     }
//                     if (ImGui::Button("Save project")) { save_project("test.3dg"); }
//                     ImGui::Button("Export project to obj");
//                     ImGui::EndMenu();
//                 }

//                 ImGui::EndMenuBar();
//             }

//             imgui_mouse_wheel                   = ImGui::GetIO().MouseWheel;
//             auto &style                         = ImGui::GetStyle();
//             static auto left_split_width_factor = 0.7f;
//             auto left_split_size                = ImGui::GetContentRegionAvail();
//             left_split_size.x *= left_split_width_factor;
//             if (ImGui::BeginChild("left split", left_split_size, true)) {
//                 auto left_split_content = ImGui::GetContentRegionAvail();
//                 canvas_size = ImVec2(left_split_content.x, left_split_content.x / win.aspect());

//                 if (ImGui::BeginChild("canvas", canvas_size, true)) {
//                     glFinish();
//                     auto handle = texture_render->handle();
//                     image_size  = ImGui::GetContentRegionAvail();

//                     auto _pos = ImGui::GetCursorPos();
//                     ImGui::InvisibleButton("is canvas hovered", ImVec2(image_size.x, image_size.y));
//                     if (ImGui::IsItemHovered()) {
//                         orb_cam->set_mouse_state(false);
//                         orb_cam->set_wheel_state(false);
//                     } else {
//                         orb_cam->set_mouse_state(true);
//                         orb_cam->set_wheel_state(true);
//                     }

//                     ImGui::SetCursorPos(_pos);

//                     if (needs_a_resize) {
//                         needs_a_resize = false;
//                         resize_fbo(image_size.x, image_size.y);
//                     }

//                     ImGui::Image((void *)(handle), image_size);
//                 }
//                 ImGui::EndChild();

//                 if (ImGui::BeginChild("logs", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar)) {
//                     if (ImGui::BeginMenuBar()) {
//                         ImGui::Text("Logs");
//                         ImGui::EndMenuBar();
//                     }
//                 }
//                 ImGui::EndChild();
//             }
//             ImGui::EndChild();
//             ImGui::SameLine();
//             ImGui::SetCursorPosX(ImGui::GetCursorPosX() - style.ItemSpacing.x);
//             ImGui::Button("##split factor slider", ImVec2(10.0f, left_split_size.y));
//             if (ImGui::IsItemActive()) {
//                 left_split_width_factor += ImGui::GetMouseDragDelta(0, 0.01).x / (float)win.width();
//                 left_split_width_factor = glm::clamp(left_split_width_factor, 0.5f, 0.8f);
//                 ImGui::ResetMouseDragDelta();
//             }
//             if (ImGui::IsItemDeactivated() && ImGui::IsMouseReleased(0)) {
//                 resize_fbo((uint32_t)image_size.x, (uint32_t)image_size.y);
//             }
//             ImGui::SameLine();
//             ImGui::SetCursorPosX(ImGui::GetCursorPosX() - style.ItemSpacing.x);
//             if (ImGui::BeginChild("right split", ImGui::GetContentRegionAvail(), true)) {
//                 enum class SelectedTab { Functions, Constants, Sliders, Settings };
//                 static SelectedTab selected_tab = SelectedTab::Functions;
//                 if (ImGui::BeginTabBar("shader tabs")) {
//                     // clang-format off
//                     static const char* tabs[]{ "Functions", "Constants", "Sliders", "Settings"};
//                     static int tab_count = sizeof(tabs) / sizeof(const char*);

//                     for(int i=0;i<tab_count; ++i) {
//                         bool is_selected = (int)selected_tab == i;
//                         if(is_selected) { ImGui::PushStyleColor(ImGuiCol_Tab, ImGui::GetStyle().Colors[ImGuiCol_TabActive]); }
//                         if (ImGui::TabItemButton(tabs[i])) { selected_tab = (SelectedTab)i; }
//                         if(is_selected) { ImGui::PopStyleColor(); }
                        
//                     }

//                     // clang-format on
//                     ImGui::EndTabBar();
//                 }

//                 const auto add_variable = [](auto &vec, const auto &val) {
//                     std::string name = "_" + std::to_string(vec.size() + 1);
//                     vec.push_back({name, val});
//                 };

//                 static int type_to_delete = -1;
//                 static int idx_to_delete  = -1;
//                 const auto draw_data      = [&](auto idx, auto &data, auto &storage) {
//                     auto type = 0;
//                     std::string name, value;
//                     using T = std::remove_cvref_t<decltype(data)>;
//                     if constexpr (std::same_as<T, Function>) {
//                         type  = 1;
//                         value = data.value;
//                     } else if (std::same_as<T, Constant>) {
//                         type  = 2;
//                         value = std::to_string(data.value);
//                     } else if (std::same_as<T, Slider>) {
//                         type  = 3;
//                         value = std::to_string(data.value);
//                     }
//                     name = data.name;

//                     static int editing_idx           = -1;
//                     static int editing_col           = -1;
//                     static int should_focus_on_input = 0;
//                     int is_any_clicked               = 0;
//                     int is_any_hovered               = 0;

//                     if (type < 3) {
//                         ImGui::TableSetColumnIndex(0);
//                         if (editing_idx == idx && editing_col == 1) {
//                             if (should_focus_on_input) {
//                                 ImGui::SetKeyboardFocusHere();
//                                 should_focus_on_input = 0;
//                             }
//                             ImGui::InputText("##name_to_edit", &data.name);
//                             if (ImGui::IsItemDeactivated()) {
//                                 editing_idx         = -1;
//                                 needs_recompilation = true;
//                             }
//                         } else {
//                             is_any_clicked |= ImGui::Selectable(name.c_str()) << 0;
//                             if (ImGui::BeginPopupContextItem()) {
//                                 if (ImGui::Button("Delete")) {
//                                     type_to_delete = type;
//                                     idx_to_delete  = idx;
//                                 }

//                                 ImGui::EndPopup();
//                             }
//                         }
//                         is_any_hovered |= ImGui::IsItemHovered() << 0;
//                         ImGui::TableSetColumnIndex(1);
//                         if (editing_idx == idx && editing_col == 2) {
//                             if (should_focus_on_input) {
//                                 ImGui::SetKeyboardFocusHere();
//                                 should_focus_on_input = 0;
//                             }
//                             if (type == 1) {
//                                 ImGui::InputText("##value_to_edit", &((Function &)data).value);
//                             } else {
//                                 ImGui::InputFloat("##value_to_edit", &((Constant &)data).value);
//                             }
//                             if (ImGui::IsItemDeactivated()) {
//                                 editing_idx         = -1;
//                                 needs_recompilation = true;
//                             }
//                         } else {
//                             is_any_clicked |= ImGui::Selectable(value.c_str()) << 1;
//                         }
//                         is_any_hovered |= ImGui::IsItemHovered() << 1;
//                     } else if (type == 3) {
//                         ImGui::TableSetColumnIndex(0);
//                         if (editing_idx == idx && editing_col == 1) {
//                             if (should_focus_on_input) {
//                                 ImGui::SetKeyboardFocusHere();
//                                 should_focus_on_input = 0;
//                             }
//                             ImGui::InputText("##name_to_edit", &data.name);
//                             if (ImGui::IsItemDeactivated()) {
//                                 editing_idx         = -1;
//                                 needs_recompilation = true;
//                             }
//                         } else {
//                             is_any_clicked
//                                 |= ImGui::Selectable(
//                                        name.c_str(),
//                                        false,
//                                        0,
//                                        ImVec2(0.0, ImGui::GetTextLineHeightWithSpacing() * 2.0))
//                                    << 0;
//                         }
//                         is_any_hovered |= ImGui::IsItemHovered() << 0;
//                         ImGui::TableSetColumnIndex(1);
//                         ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.3);
//                         ImGui::InputFloat("Min", &((Slider &)data).min);
//                         ImGui::SameLine();
//                         ImGui::InputFloat("Max", &((Slider &)data).max);
//                         ImGui::PopItemWidth();
//                         ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
//                         is_any_clicked |= ImGui::SliderFloat("##slider",
//                                                              (float *)&data.value,
//                                                              ((Slider &)data).min,
//                                                              ((Slider &)data).max)
//                                           << 1;
//                         is_any_hovered |= ImGui::IsItemHovered() << 1;
//                         if (ImGui::BeginPopupContextItem()) {
//                             if (ImGui::Button("Delete")) {
//                                 type_to_delete = type;
//                                 idx_to_delete  = idx;
//                             }

//                             ImGui::EndPopup();
//                         }
//                     }

//                     static int dragged_idx         = -1;
//                     static int prev_is_any_hovered = is_any_hovered;
//                     if (is_any_hovered && ImGui::IsMouseClicked(0)) {
//                         dragged_idx = static_cast<int>(idx);
//                     }
//                     auto stopped_hovering = !is_any_hovered && prev_is_any_hovered;
//                     if (dragged_idx == idx) { prev_is_any_hovered = is_any_hovered; }

//                     if (idx == dragged_idx && stopped_hovering && ImGui::IsMouseDragging(0, 0.1f)) {
//                         auto delta   = ImGui::GetMouseDragDelta().y;
//                         auto new_idx = idx + (int)(delta > 0.0f ? 1 : delta < 0.0f ? -1 : 0);
//                         new_idx      = glm::clamp((int)new_idx, 0, (int)storage.size() - 1);
//                         ImGui::ResetMouseDragDelta();
//                         if (idx == new_idx) { return; }
//                         T temp_data      = data;
//                         storage[idx]     = storage[new_idx];
//                         storage[new_idx] = temp_data;
//                         dragged_idx      = new_idx;
//                     }

//                     if (dragged_idx > -1 && ImGui::IsMouseReleased(0)) { dragged_idx = -1; }

//                     if (editing_idx == -1 && is_any_hovered && ImGui::IsMouseDoubleClicked(0)) {
//                         editing_idx           = idx;
//                         editing_col           = is_any_hovered;
//                         should_focus_on_input = 1;
//                     }
//                 };

//                 if (idx_to_delete > -1) {
//                     switch (type_to_delete) {
//                     case 1: functions.erase(functions.begin() + idx_to_delete); break;
//                     case 2: constants.erase(constants.begin() + idx_to_delete); break;
//                     case 3: sliders.erase(sliders.begin() + idx_to_delete); break;
//                     }
//                     idx_to_delete = -1;
//                 }

//                 if (ImGui::BeginChild("right_split_content")) {
//                     switch (selected_tab) {
//                     case SelectedTab::Functions: {
//                         if (ImGui::Button("Add new function")) {
//                             add_variable(functions, "sin(x)");
//                         }

//                         display_2col_list(functions, draw_data);
//                         break;
//                     }
//                     case SelectedTab::Constants: {
//                         if (ImGui::Button("Add new constant")) { add_variable(constants, 0.0f); }
//                         display_2col_list(constants, draw_data);
//                         break;
//                     }
//                     case SelectedTab::Sliders: {
//                         if (ImGui::Button("Add new slider")) { add_variable(sliders, 0.0f); }
//                         display_2col_list(sliders, draw_data);
//                         break;
//                     }
//                     case SelectedTab::Settings: {
//                         draw_settings_panel();
//                         break;
//                     }
//                     }
//                 }
//                 ImGui::EndChild();
//             }
//             ImGui::EndChild();
//         }

//         ImGui::End();
//     });

//     eng.on_update.connect([&] {
//         if (needs_recompilation) {
//             recompile_shader();
//             needs_recompilation = false;
//         }

//         orb_cam->update();
//         shader->use();
//         shader->set("m", glm::scale(glm::mat4{1.0f}, glm::vec3{1.0}));
//         shader->set("v", orb_cam->view_matrix());
//         shader->set("p", orb_cam->projection_matrix());
//         shader->set("TIME", (float)glfwGetTime());
//         shader->set("size", (float)plane_setttings.bounds);
//         shader->set("detail", (float)plane_setttings.detail);
//         for (const auto &s : sliders) { shader->set(s.name, s.value); }

//         vao->bind();
//         fbo->bind();
//         glViewport(0, 0, image_size.x, image_size.y);
//         glClearColor(background_color.x, background_color.y, background_color.z, 1.0f);
//         win.set_clear_flags(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//         win.clear_framebuffer();
//         glEnable(GL_DEPTH_TEST);
//         glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

//         glDrawElementsInstanced(
//             GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, plane_setttings.detail * plane_setttings.detail);

//         line_shader->use();
//         line_shader->set("p", orb_cam->projection_matrix());
//         line_shader->set("v", orb_cam->view_matrix());
//         line_shader->set("span", (float)plane_setttings.bounds);
//         glLineWidth(3.0);
//         line_vao->bind();
//         line_shader->set("color", glm::vec3{1.0, 0.0, 0.0});
//         glDrawArrays(GL_LINES, 0, 2);
//         line_shader->set("color", glm::vec3{0.0, 1.0, 0.0});
//         glDrawArrays(GL_LINES, 2, 2);
//         line_shader->set("color", glm::vec3{0.0, 0.0, 1.0});
//         glDrawArrays(GL_LINES, 4, 2);
//         glLineWidth(1.0);

//         glBindFramebuffer(GL_FRAMEBUFFER, 0);
//         glViewport(0, 0, win.width(), win.height());
//     });
//     recompile_shader();

//     ImGui::GetIO().WantSaveIniSettings = true;
//     ImGui::GetIO().IniSavingRate       = 1.0f;

//     eng.start();
//     Engine::exit();

    return 0;
}

static void start_application(const char* window_title, uint32_t window_width, uint32_t window_height) {
    // initialise opengl and create window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_COMPAT_PROFILE, GLFW_OPENGL_FORWARD_COMPAT);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    app_state.window = g3d::Window{window_title, window_width, window_height};
    app_state.window.pglfw_window = glfwCreateWindow(app_state.window.width, app_state.window.height, app_state.window.title, 0, 0);
    assert(app_state.window.pglfw_window && "Could not create the window");

    glfwMakeContextCurrent(app_state.window.pglfw_window);
    
    auto glad_init_result = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    assert(glad_init_result && "Could not initialize OpenGL");

    // initialise ImGui 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(app_state.window.pglfw_window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");
    ImGui::StyleColorsDark();
}

static void terminate_application() {
    
}
// static void draw_settings_panel() {
//     if (ImGui::CollapsingHeader("Plane details")) {
//         ImGui::SliderInt("Detail", (int *)&plane_setttings.detail, 0, 200);
//         ImGui::SameLine();
//         ImGui::TextDisabled("(?)");
//         if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
//             ImGui::SetTooltip("Set into how many small squares the plane should be divided.");
//         }
//         ImGui::SliderInt("Bounds", (int *)&plane_setttings.bounds, 0, 200);
//         ImGui::TextDisabled("(?)");
//         if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
//             ImGui::SetTooltip("Set how far the plane is stretched. 2 means the plane spans in both x and z from -2 to 2");
//         }
//     }
//     if (ImGui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
//         ImGui::ColorEdit4("Background", &background_color.x);
//     }
// }

// template <typename T, typename DRAW_CB>
// static void display_2col_list(std::vector<T> &data, DRAW_CB draw_callback) {

//     const auto NUM_COLS    = 2;
//     const auto TABLE_FLAGS = ImGuiTableFlags_BordersH | ImGuiTableFlags_RowBg
//                              | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
//     const auto TABLE_SIZE = ImGui::GetContentRegionAvail();

//     if (ImGui::BeginTable("2ColListDT", NUM_COLS, TABLE_FLAGS, TABLE_SIZE)) {
//         ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
//         ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
//         ImGui::TableSetupScrollFreeze(NUM_COLS, 1);
//         ImGui::TableHeadersRow();

//         for (auto i = 0llu; i < data.size(); ++i) {
//             ImGui::PushID(i + 1);
//             ImGui::TableNextRow();
//             ImGui::TableSetColumnIndex(0);
//             draw_callback(i, data.at(i), data);
//             ImGui::PopID();
//         }

//         ImGui::EndTable();
//     }
// }

// // clang-format off
// static void save_project(const char *file_name) {
//     std::ofstream file;
//     file.open(file_name, std::ofstream::out);
//     if (file.is_open() == false) { std::cerr << "File is not opened!"; return; }
    
//     std::stringstream content;
//     content << "[Functions]\n"; for (const auto &f : functions) { content << f.name << ' ' << f.value << '\n'; }
//     content << "[Constants]\n"; for (const auto &f : constants) { content << f.name << ' ' << f.value << '\n'; }
//     content << "[Sliders]\n";   for (const auto &f : sliders)   { content << f.name << ' ' << f.value << ' ' << f.min << ' ' << f.max << '\n'; }

//     content << "[Color Settings]\n";
//     /*Background color*/ for(int i=0; i<4; ++i) {content << background_color[i]; if(i<3)content<<' ';} content << '\n';

//     content << "[Rendering Settings]\n";
//     /*Plane settings*/ content << plane_setttings.bounds << ' ' << plane_setttings.detail;

//     file << content.rdbuf(); file.close();
// }
// // clang-format on

// static void load_project(const char *file_name) {
//     std::ifstream file{file_name};
//     if (file.is_open() == false) { throw std::runtime_error{"File could not be opened"}; }

//     std::string line;
//     int resource_id = -1;
//     functions.clear();
//     constants.clear();
//     sliders.clear();
//     while (std::getline(file, line)) {
//         if (line.starts_with("[Functions]")) {
//             resource_id = 0;
//             continue;
//         } else if (line.starts_with("[Constants]")) {
//             resource_id = 1;
//             continue;
//         } else if (line.starts_with("[Sliders]")) {
//             resource_id = 2;
//             continue;
//         } else if (line.starts_with("[Color Settings]")) {
//             resource_id = 3;
//             continue;
//         } else if (line.starts_with("[Rendering Settings]")) {
//             resource_id = 4;
//             continue;
//         }

//         if (resource_id == -1 || resource_id > 4) {
//             throw std::runtime_error{"Error while reading the project file - label not found: "
//                                      + line};
//         }

//         std::stringstream ssline{line};
//         switch (resource_id) {
//         case 0: {
//             Function f;
//             ssline >> f.name >> f.value;
//             functions.push_back(f);
//             break;
//         }
//         case 1: {
//             Constant c;
//             ssline >> c.name >> c.value;
//             constants.push_back(c);
//             break;
//         }
//         case 2: {
//             Slider s;
//             ssline >> s.name >> s.value >> s.min >> s.max;
//             sliders.push_back(s);
//             break;
//         }
//         case 3: {
//             ssline >> background_color.x >> background_color.y >> background_color.z
//                 >> background_color.w;
//             break;
//         }
//         case 4: {
//             ssline >> plane_setttings.bounds >> plane_setttings.detail;
//             break;
//         }
//         }
//     }
// }