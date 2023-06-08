#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <map>
#include <concepts>
#include <ShlObj_core.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_stdlib.h>

#include <orbital_camera.hpp>
#include <renderer.hpp>

struct Function {
    std::string name, value;
};
struct Slider {
    Slider() = default;
    Slider(const std::string &name, float value) : name{name}, value{value} {}

    std::string name;
    float value{0.f}, min{0.f}, max{1.0f};
};
struct Constant {
    std::string name;
    float value{0.f};
};

struct AppState {
    g3d::Window window;
    g3d::OrbitalCamera camera;
    g3d::Framebuffer framebuffer_main;

    std::vector<Function> functions;
    std::vector<Slider> sliders;
    std::vector<Constant> constants;
    
    std::vector<std::string> logs;
    bool needs_recompilation = false;
    uint32_t* heights_buffer=nullptr;
    bool log_list_scroll_down = false;

    struct PlaneSettings {
        uint32_t detail{3}, bounds{1};
        float grid_step = 1.0;
        float grid_line_thickness = 0.98;
        bool is_detail_affecting_grid_step  = true;
    } plane_settings;

    struct ColorSettings {
        glm::vec4 color_background  = glm::vec4{33,  35,  38,  255} / glm::vec4{255};
        glm::vec4 color_grid        = glm::vec4{94,  94,  94,  255} / glm::vec4{255};
        glm::vec4 color_plane       = glm::vec4{135, 135, 135, 255} / glm::vec4{255};
        glm::vec4 color_plane_grid  = glm::vec4{228, 159, 61,  255} / glm::vec4{255};
    } color_settings;

    struct RenderSettings {
        bool is_grid_rendered               = true;
        bool is_plane_grid_rendered         = true;
    } render_settings;
} app_state;

static void start_application(const char* window_title, uint32_t window_width, uint32_t window_height);
static void terminate_application();
static void on_window_resize(GLFWwindow*, int, int);

static void imgui_newframe();
static void imgui_renderframe();

static void resize_main_frambuffer(uint32_t width, uint32_t height);
static void set_rendering_state_opengl(const g3d::RenderState&);
static void create_plane_shader_source_and_compile(g3d::HandleProgram program, g3d::HandleShader &vertex_shader, g3d::HandleShader &fragment_shader);
static void create_grid_shader_source_and_compile (g3d::HandleProgram program, g3d::HandleShader &vertex_shader, g3d::HandleShader &fragment_shader);
static void create_compute_shader(g3d::HandleProgram program, g3d::HandleShader &compute_shader);
static void recalculate_plane_height_field(g3d::HandleProgram program, g3d::HandleBuffer *height_buffer, uint32_t *current_size);
static void draw_gui();

static void uniform1f(uint32_t program, const char* name, float value);
static void uniform3f(uint32_t program, const char* name, glm::vec3 value);
static void uniformm4(uint32_t program, const char* name, const glm::mat4& value);

static void save_project(const char *file_name);
static void load_project(const char *file_name);
static void export_to_obj(const std::string& file_name, g3d::HandleBuffer buffer);
static void log_list_add_message(const std::string& msg);

int main() {
    start_application("3DCalc", 1280, 960);

    g3d::HandleFramebuffer framebuffer_main;
    g3d::HandleTexture texture_fmain_color;
    g3d::HandleTexture texture_fmain_depth_stencil;

    g3d::HandleProgram program_compute; g3d::HandleShader shader_compute;
    g3d::HandleProgram program_plane;   g3d::HandleShader shader_plane_vert;    g3d::HandleShader shader_plane_frag;
    g3d::HandleProgram program_line;    g3d::HandleShader shader_line_vert;     g3d::HandleShader shader_line_frag;
    g3d::HandleProgram program_grid;    g3d::HandleShader shader_grid_vert;     g3d::HandleShader shader_grid_frag;

    program_compute = glCreateProgram();
    g3d::HandleVao vao_plane;
    g3d::HandleVao vao_line;
    g3d::HandleBuffer vbo_plane, ebo_plane, vbo_heights_plane;
    g3d::HandleBuffer vbo_line;
    glCreateFramebuffers(1, &framebuffer_main);
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_fmain_color);
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_fmain_depth_stencil);
    glTextureStorage2D(texture_fmain_color, 1, GL_RGB8, app_state.window.width, app_state.window.height);
    glTextureStorage2D(texture_fmain_depth_stencil, 1, GL_DEPTH24_STENCIL8, app_state.window.width, app_state.window.height);
    glNamedFramebufferTexture(framebuffer_main, GL_COLOR_ATTACHMENT0, texture_fmain_color, 0);
    glNamedFramebufferTexture(framebuffer_main, GL_DEPTH_STENCIL_ATTACHMENT, texture_fmain_depth_stencil, 0);
    glNamedFramebufferDrawBuffer(framebuffer_main, GL_COLOR_ATTACHMENT0);
    assert((glCheckNamedFramebufferStatus(framebuffer_main, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) && "Main framebuffer initialisation error.");
    app_state.heights_buffer = &vbo_heights_plane;

    program_plane = glCreateProgram();  shader_plane_vert = glCreateShader(GL_VERTEX_SHADER); shader_plane_frag = glCreateShader(GL_FRAGMENT_SHADER);
    program_grid  = glCreateProgram();  shader_grid_vert  = glCreateShader(GL_VERTEX_SHADER); shader_grid_frag  = glCreateShader(GL_FRAGMENT_SHADER);

    glCreateVertexArrays(1, &vao_plane);
    glCreateBuffers(1, &vbo_plane);
    glCreateBuffers(1, &ebo_plane);
    glCreateBuffers(1, &vbo_heights_plane);
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

     {
        float vbo[] {-1.0,  1.0, 1.0,  1.0, -1.0, -1.0, 1.0, -1.0 };
        unsigned ebo[]{0, 1, 2, 2, 1, 3};
        float lineverts[]{ -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0,-1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0,-1.0, 0.0, 0.0, 1.0 };

        glNamedBufferStorage(vbo_plane, sizeof(vbo), vbo, 0);
        glNamedBufferStorage(ebo_plane, sizeof(ebo), ebo, 0);
        glNamedBufferStorage(vbo_line,  sizeof(lineverts), lineverts, 0);
    }

    g3d::Texture texture_fmain_color_wrapped {
        .handle = texture_fmain_color,
        .target = GL_TEXTURE_2D, .format = GL_RGB8,
        .width = app_state.window.width, .height = app_state.window.height
    };
    g3d::Texture texture_fmain_depth_stencil_wrapped {
        .handle = texture_fmain_depth_stencil,
        .target = GL_TEXTURE_2D, .format = GL_DEPTH24_STENCIL8,
        .width = app_state.window.width, .height = app_state.window.height
    };


    // App State Initialisation    
    app_state.camera = g3d::OrbitalCamera{&app_state.window, g3d::OrbitalCameraSettings{90.0f, 0.01f, 100.0f}};
    app_state.framebuffer_main = g3d::Framebuffer{
        .handle = framebuffer_main,
        .textures = {{GL_COLOR_ATTACHMENT0,          &texture_fmain_color_wrapped},
                     {GL_DEPTH_STENCIL_ATTACHMENT,   &texture_fmain_depth_stencil_wrapped}
        }
    };
    // Function "f" shall always be present
    app_state.functions.push_back(Function{"f", "sin(x)"});

    // Rendering States
    g3d::RenderState plane_render_state{ 
        .vao = vao_plane, .program = program_plane 
    };
    g3d::RenderState grid_render_state{ 
        .vao = vao_line, .program = program_grid, .line_width = 2.0f
    };

    auto &window = app_state.window;
    
    create_plane_shader_source_and_compile(program_plane, shader_plane_vert, shader_plane_frag);
    create_grid_shader_source_and_compile(program_grid, shader_grid_vert, shader_grid_frag);
    create_compute_shader(program_compute, shader_compute);
    uint32_t current_buffer_size=0;
    while(glfwWindowShouldClose(window.pglfw_window) == false) {
        glfwPollEvents();
        imgui_newframe();
        app_state.camera.update();

        if(app_state.needs_recompilation) {
            app_state.needs_recompilation = false;
            try {
                create_compute_shader(program_compute, shader_compute);
            } catch(const std::exception& err) {
                log_list_add_message(err.what());
            }
        }

        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, app_state.framebuffer_main.handle);
        glViewport(0, 0, app_state.framebuffer_main.textures[0].second->width, app_state.framebuffer_main.textures[0].second->height);
        auto &bc = app_state.color_settings.color_background;
        glClearColor(bc.r, bc.g, bc.b, bc.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        if (app_state.render_settings.is_grid_rendered) {
            set_rendering_state_opengl(grid_render_state);
            uniformm4(program_grid, "v", app_state.camera.view_matrix());
            uniformm4(program_grid, "p", app_state.camera.projection_matrix());
            uniform1f(program_grid, "bounds", app_state.plane_settings.bounds);
            uniform3f(program_grid, "user_color", app_state.color_settings.color_grid);
            glDrawArraysInstanced(GL_LINES, 0, 2, app_state.plane_settings.bounds * 2 + 1);
            glDrawArraysInstanced(GL_LINES, 4, 2, app_state.plane_settings.bounds * 2 + 1);
        }

        glUseProgram(program_compute);
        uniform1f(program_compute, "detail", app_state.plane_settings.detail);
        uniform1f(program_compute, "bounds", app_state.plane_settings.bounds);
        uniform1f(program_compute, "TIME", (float)glfwGetTime());
        for(const auto& s : app_state.sliders) { uniform1f(program_plane, s.name.c_str(), s.value); }
        recalculate_plane_height_field(program_compute, &vbo_heights_plane, &current_buffer_size); 
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        set_rendering_state_opengl(plane_render_state);
        uniformm4(program_plane, "v", app_state.camera.view_matrix());
        uniformm4(program_plane, "p", app_state.camera.projection_matrix());
        uniform1f(program_plane, "detail", app_state.plane_settings.detail);
        uniform1f(program_plane, "size", app_state.plane_settings.bounds);
        uniform3f(program_plane, "user_color", app_state.color_settings.color_plane);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, app_state.plane_settings.detail * app_state.plane_settings.detail);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, app_state.window.width, app_state.window.height);
        glClear(GL_COLOR_BUFFER_BIT);
        draw_gui();
        
        imgui_renderframe();
        glfwSwapBuffers(window.pglfw_window);   
        window.has_just_resized = false;
    }
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

    glfwSetFramebufferSizeCallback(app_state.window.pglfw_window, on_window_resize);
}

static void terminate_application() {
    
}
static void imgui_newframe() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
static void imgui_renderframe() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
static void resize_main_frambuffer(uint32_t width, uint32_t height) {
    for(auto& [attachment, t] : app_state.framebuffer_main.textures) {
        t->width = width;
        t->height= height;

        glDeleteTextures(1, &t->handle);
        glCreateTextures(t->target, 1, &t->handle);
        glTextureStorage2D(t->handle, 1, t->format, t->width, t->height);
        glNamedFramebufferTexture(app_state.framebuffer_main.handle, attachment, t->handle, 0);
    }
}
static void set_rendering_state_opengl(const g3d::RenderState& state) {
    glBindVertexArray(state.vao);
    glUseProgram(state.program);
    glLineWidth(state.line_width);
}
static void create_plane_shader_source_and_compile(g3d::HandleProgram program, g3d::HandleShader& vertex_shader, g3d::HandleShader& fragment_shader) {

    std::string shader_source;
    shader_source = R"glsl(
        #version 460 core
        layout(location=0) in vec2 in_pos;
        layout(std430, binding=0) buffer HeightField {
            float values[];
        };
        uniform mat4 p;
        uniform mat4 v;
        uniform float TIME;
        uniform float detail;
        uniform float size;

        out vec3 vout_pos;
        out vec3 vout_norm;

        void main() {
            uint gx = (gl_InstanceID) % uint(detail);
            uint gy = (gl_InstanceID) / uint(detail);
            uint gidx = (gy+uint(in_pos.y)) * uint(detail+1) + (gx+uint(in_pos.x));
            float height = values[gidx];
            vec2 vpos = in_pos * 0.5 + 0.5;
            vpos = vpos * (2.0*size / detail);
            vpos = vpos - size;
            vpos.x += (2.0*size)/detail * float(gx);
            vpos.y += (2.0*size)/detail * float(gy);
            vout_pos = vec3(vpos.x, height, vpos.y);

            vec3 a = vout_pos;
            vec3 b = vec3(vpos.x + (2.0*size)/detail, values[gidx+1], vpos.y);
            vec3 c = vec3(vpos.x, values[gidx+uint(detail+1.0)], vpos.y + (2.0*size)/detail);

            vout_norm = normalize(cross(c - a, b - a));

            gl_Position = p * v * vec4(vout_pos, 1.0);
        }
    )glsl";
        std::string frag_src = R"glsl(
            #version 460 core
            out vec4 FRAG_COLOR;
            in vec3 vout_pos;
            flat in vec3 vout_norm;
            uniform vec3 user_color;

            void main() { 
                float att = clamp(dot(vec3(0.0,1.0,0.0), vout_norm), 0.3, 1.0);
                FRAG_COLOR = vec4(user_color * att, 1.0);
            }
        
        )glsl";

    const auto *vertex_source = shader_source.c_str();
    const auto *fragment_source   = frag_src.c_str();

    g3d::HandleShader new_vertex_shader, new_fragment_shader;
    new_vertex_shader   = glCreateShader(GL_VERTEX_SHADER); 
    new_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER); 
    glShaderSource(new_vertex_shader, 1, &vertex_source, 0);
    glShaderSource(new_fragment_shader, 1, &fragment_source, 0);
    glCompileShader(new_vertex_shader); glCompileShader(new_fragment_shader);

    glDetachShader(program, vertex_shader); glDetachShader(program, fragment_shader);
    glDeleteShader(vertex_shader); glDeleteShader(fragment_shader);
    glAttachShader(program, new_vertex_shader); glAttachShader(program, new_fragment_shader);
    glLinkProgram(program);
    vertex_shader   = new_vertex_shader; 
    fragment_shader = new_fragment_shader;
}

static void create_grid_shader_source_and_compile(g3d::HandleProgram program, g3d::HandleShader &vertex_shader, g3d::HandleShader &fragment_shader) {
    const char *vertex_source = R"glsl(
        #version 460 core
        layout(location=0) in vec3 pos;
        uniform mat4 v;
        uniform mat4 p;
        uniform float bounds;

        void main() {
            const vec3 offset_dir = abs(cross(pos, vec3(0.0,1.0,0.0)));
            vec3 offset = offset_dir * bounds;
            vec3 calc_pos = pos*bounds - offset + offset_dir * float(gl_InstanceID);
            gl_Position = p * v * vec4(calc_pos, 1.0);
        }

    )glsl";
    const char *fragment_source = R"glsl(
        #version 460 core
        out vec4 FRAG_COLOR;
        uniform vec3 user_color;
        void main() { FRAG_COLOR = vec4(user_color, 1.0); }
    )glsl";

    glShaderSource(vertex_shader, 1, &vertex_source, 0); glShaderSource(fragment_shader, 1, &fragment_source, 0);
    glCompileShader(vertex_shader); glCompileShader(fragment_shader);
    glAttachShader(program, vertex_shader); glAttachShader(program, fragment_shader);
    glLinkProgram(program);
}

static void create_compute_shader(g3d::HandleProgram program, g3d::HandleShader &compute_shader) {
    glDetachShader(program, compute_shader);
    glDeleteShader(compute_shader);

    compute_shader = glCreateShader(GL_COMPUTE_SHADER);

    std::string compute_source = R"glsl(
        #version 460 core
        layout(local_size_x=32, local_size_y=32, local_size_z=1) in;
        layout(std430, binding=0) buffer height_field { float values[]; };
        uniform float detail;
        uniform float bounds;
        uniform float TIME;
        void main() {
            uint vx_in_row = uint(detail)+1;
            uint gx = gl_GlobalInvocationID.x;
            uint gy = gl_GlobalInvocationID.y;
            if (gx >= vx_in_row || gy >= vx_in_row) {return;}
            uint idx = gy*vx_in_row + gx;
            vec2 pos = vec2(float(gx), float(gy));
            pos = pos / detail;
            pos = pos * 2.0*bounds - bounds; // <-bounds, bounds>

            values[gy*vx_in_row + gx] = f(pos.x, pos.y);
        }
    )glsl";


    std::string forward_declarations;
    std::string function_definitions;
    std::string uniforms;
    std::string consts;

    auto& functions = app_state.functions;
    auto& constants = app_state.constants;
    auto& sliders   = app_state.sliders;

    for (const auto &f : functions) {
        forward_declarations += "float " + f.name + "(float,float);\n";
        function_definitions += "float " + f.name + "(float x,float z){return " + f.value + ";}\n"; }
    for (const auto &c : constants) { consts += "const float " + c.name + "=" + std::to_string(c.value) + ";\n"; }
    for (const auto &s : sliders) { uniforms += "uniform float " + s.name + ";\n"; } 

    auto forward_dec_idx = 0u;
    for (auto new_line_count=0u; new_line_count != 4; ++forward_dec_idx) {
        if(compute_source.at(forward_dec_idx) == '\n') { ++new_line_count; }
    }
    forward_declarations += consts;
    forward_declarations += uniforms;

    compute_source.insert(compute_source.begin() + forward_dec_idx, forward_declarations.begin(), forward_declarations.end());
    compute_source += function_definitions;
    const auto *compute_cstr = compute_source.c_str();
    glShaderSource(compute_shader, 1, &compute_cstr, 0);
    glCompileShader(compute_shader);


    char* error_log = nullptr;
    const auto get_shader_compilation_error_message = [&](g3d::HandleShader shader){
        int compile_status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

        if(compile_status != GL_TRUE) {
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &compile_status);
            error_log = (char*)malloc(compile_status);
            assert(error_log != nullptr && "Could not allocate memory for shader error log. Possibly the system has no free memory.");
            glGetShaderInfoLog(shader, compile_status, &compile_status, error_log);
        }
    };

    get_shader_compilation_error_message(compute_shader);
    if(error_log != nullptr) {
        std::string error_message;
        error_message += error_log;
        delete[] error_log;
        throw std::runtime_error{error_message};
    }

    glAttachShader(program, compute_shader);
    glLinkProgram(program);
}
static void recalculate_plane_height_field(g3d::HandleProgram program, g3d::HandleBuffer *height_buffer, uint32_t *current_size) {
    const auto vertex_count = (unsigned)glm::pow(app_state.plane_settings.detail+1, 2);
    if(vertex_count > *current_size) {
        *current_size = vertex_count; 
        glDeleteBuffers(1, height_buffer);
        glCreateBuffers(1, height_buffer);
        glNamedBufferStorage(*height_buffer, vertex_count * sizeof(float), 0, GL_MAP_READ_BIT);
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *height_buffer);
    //glUseProgram(program);
    const float compute_num = (float)(app_state.plane_settings.detail+1) / 32.0f;
    const uint32_t compute_num_uint = glm::ceil(compute_num);
    glDispatchCompute(compute_num_uint, compute_num_uint, 1);
}

static void draw_menu_bar();
static void draw_left_child();
static void draw_right_child();
static void draw_gui() {
    auto main_screen_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar;
    auto &window = app_state.window;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(window.width, window.height));
    if (ImGui::Begin("main screen", 0, main_screen_flags)) {
        draw_menu_bar();
        draw_left_child(); ImGui::SameLine(); draw_right_child();
    }
    ImGui::End();
}
static void on_window_resize(GLFWwindow*, int width, int height) {
    app_state.window.has_just_resized = true;
    app_state.window.width  = static_cast<uint32_t>(width);
    app_state.window.height = static_cast<uint32_t>(height);
}


static void draw_menu_bar() {
    if (ImGui::BeginMenuBar()) {
        if(ImGui::BeginMenu("Project...")) {
            static std::string file_name;
            static bool no_name_error = false;
            if (ImGui::Button("Open project")) {
                nfdchar_t *outPath = NULL;
                nfdresult_t result = NFD_OpenDialog("3dg", std::filesystem::current_path().string().c_str(), &outPath);
                if (result == NFD_OKAY) {
                    load_project(outPath);
                    file_name = outPath;
                    file_name = file_name.substr(file_name.rfind('\\')+1, file_name.rfind('.') - file_name.rfind('\\')-1);
                    app_state.needs_recompilation = true;
                }
            }
        
            if (ImGui::Button("Save project")) { 
                ImGui::OpenPopup("save_project_popup");
            }
            if(ImGui::BeginPopup("save_project_popup")) {
                ImGui::Text("Filename:"); ImGui::SameLine();
                ImGui::PushItemWidth(160.0f);
                ImGui::InputText("##Filename", &file_name);
                const auto spacex = ImGui::GetContentRegionAvail().x;
                ImGui::Indent(spacex - 100.0f);
                if(ImGui::Button("Save", ImVec2(100.0f, 0.0f))) {
                    if (no_name_error == false && file_name.empty()) { no_name_error = true; }

                   if (no_name_error == false || file_name.empty() == false) {
                        no_name_error = false;
                        std::string file_name_with_ext = file_name + ".3dg";
                        
                        save_project(file_name_with_ext.c_str()); 
                        log_list_add_message("Project successfully saved");
                        ImGui::CloseCurrentPopup();
                   }
                }
                ImGui::PopItemWidth();
                ImGui::SetCursorPosX(10.0f);
                if (no_name_error) { ImGui::TextColored(ImVec4(255, 0,0,255), "File name field cannot be empty!"); }
                ImGui::EndPopup();
            }
            
            if(ImGui::Button("Export project to obj")) {
                ImGui::OpenPopup("export_to_obj_popup");
            }
            if(ImGui::BeginPopup("export_to_obj_popup")) {
                ImGui::Text("Filename:"); ImGui::SameLine();
                ImGui::PushItemWidth(160.0f);
                ImGui::InputText("##Filename", &file_name);
                const auto spacex = ImGui::GetContentRegionAvail().x;
                ImGui::Indent(spacex - 100.0f);
                if(ImGui::Button("Export", ImVec2(100.0f, 0.0f))) {
                    if (no_name_error == false && file_name.empty()) { no_name_error = true; }

                   if (no_name_error == false || file_name.empty() == false) {
                        no_name_error = false;
                        std::string file_name_with_ext = file_name + ".obj";
                        TCHAR path[MAX_PATH];
                        HRESULT hr = SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS    , NULL, SHGFP_TYPE_CURRENT, path);
                        if (FAILED(hr)) { assert(false); }

                        std::filesystem::path _p = std::string(path) + '\\' + file_name_with_ext;
                        log_list_add_message(std::string{"File successfully exported at: \'"} + _p.string().c_str() + '\'');
                        export_to_obj(_p.string(), *app_state.heights_buffer); 
                        ImGui::CloseCurrentPopup();
                   }
                }

                ImGui::PopItemWidth();
            ImGui::SetCursorPosX(10.0f);
                if (no_name_error) { ImGui::TextColored(ImVec4(255, 0,0,255), "File name field cannot be empty!"); }
                ImGui::EndPopup();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}
static void draw_left_child() {
    static auto left_split_width_factor = 0.7f;
    auto left_split_size                = ImGui::GetContentRegionAvail();
    left_split_size.x *= left_split_width_factor;

    ImVec2 canvas_size(1,1);
    if (ImGui::BeginChild("left split", left_split_size, true)) {
        const auto left_split_content = ImGui::GetContentRegionAvail();
        const auto aspect = (float)app_state.window.width / (float)app_state.window.height;
        canvas_size = ImVec2(left_split_content.x, left_split_content.x / aspect);

        if (ImGui::BeginChild("canvas", canvas_size, true)) {
            const auto handle = app_state.framebuffer_main.textures[0].second->handle;
            const auto image_size  = ImGui::GetContentRegionAvail();

            auto _pos = ImGui::GetCursorPos();
            ImGui::InvisibleButton("is canvas hovered", ImVec2(image_size.x, image_size.y));
            if (ImGui::IsItemHovered()) {
                app_state.camera.set_mouse_state(false);
                app_state.camera.set_wheel_state(false);
            } else {
                app_state.camera.set_mouse_state(true);
                app_state.camera.set_wheel_state(true);
            }

            ImGui::SetCursorPos(_pos);
            ImGui::Image((void *)(handle), image_size);
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("logs", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar)) {
            if (ImGui::BeginMenuBar()) {
                ImGui::Text("Logs");
                ImGui::EndMenuBar();
            }

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(255, 0, 0, 0));
            if(ImGui::BeginListBox("##log_list", ImGui::GetContentRegionAvail())) {
                uint32_t msgidx = 1;
                for(const auto& l : app_state.logs) {
                    std::string formatted_msg = std::to_string(msgidx++) + ".\t" + l + '\n';
                    ImGui::Text(formatted_msg.c_str());
                }
                if (app_state.log_list_scroll_down) {ImGui::SetScrollHereY(); app_state.log_list_scroll_down = false; }
                ImGui::EndListBox();
            }
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    auto &style = ImGui::GetStyle();
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - style.ItemSpacing.x);
    ImGui::Button("##split factor slider", ImVec2(10.0f, left_split_size.y));
    if (ImGui::IsItemActive()) {
        left_split_width_factor += ImGui::GetMouseDragDelta(0, 0.01).x / (float)app_state.window.width;
        left_split_width_factor = glm::clamp(left_split_width_factor, 0.5f, 0.8f);
        ImGui::ResetMouseDragDelta();
    }
    if ((ImGui::IsItemDeactivated() && ImGui::IsMouseReleased(0)) || app_state.window.has_just_resized) {
        resize_main_frambuffer(canvas_size.x, canvas_size.y);
        app_state.camera.on_window_resize(canvas_size.x, canvas_size.y);
    }   
}

enum class SelectedTab { Functions, Constants, Sliders, Settings };
static const char *SelectedTabNames[] { "Functions", "Constants", "Sliders", "Settings" };
               
static void draw_user_variables_table(SelectedTab tab);
static void draw_right_child() {
    static int selected_tab_idx = 0;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().WindowPadding.x);
    if(ImGui::BeginChild("right split", ImGui::GetContentRegionAvail(), true)) {
        if(ImGui::BeginTabBar("right split tabs")) {
            for(int i=0; i < 4; ++i) {
                const char *name = SelectedTabNames[i];
                const int is_tab_selected = selected_tab_idx == i;
                if(is_tab_selected) { ImGui::PushStyleColor(ImGuiCol_Tab, ImGui::GetStyle().Colors[ImGuiCol_TabActive]); }
                if(ImGui::TabItemButton(name)) { selected_tab_idx = i; }
                if(is_tab_selected) { ImGui::PopStyleColor(); }
            }
            ImGui::EndTabBar();
        }

        static const auto add_variable_button = [](const auto& btn_name, auto &vec, const auto &val) {
            if(ImGui::Button(btn_name)) {
                std::string name = "_" + std::to_string(vec.size() + 1);
                vec.push_back({name, val});
            }
        };

        switch (selected_tab_idx) {
            case 0: {
                add_variable_button("Add new function", app_state.functions, "0");                
                draw_user_variables_table(SelectedTab::Functions);
                break; 
            }
            case 1: {
                add_variable_button("Add new constant", app_state.constants, 0.0f);                
                draw_user_variables_table(SelectedTab::Constants); 
                break; 
            }
            case 2: {
                add_variable_button("Add new slider", app_state.sliders, 0.0f);                
                draw_user_variables_table(SelectedTab::Sliders); 
                break; 
            }
            case 3: {
                if (ImGui::CollapsingHeader("Color Settings")) {
                    ImGui::ColorEdit3("Background color", &app_state.color_settings.color_background.x);
                    ImGui::ColorEdit3("Plane color", &app_state.color_settings.color_plane.x);
                    ImGui::ColorEdit3("Grid color", &app_state.color_settings.color_grid.x);
                    ImGui::ColorEdit3("Plane grid color", &app_state.color_settings.color_plane_grid.x);
                }
                if (ImGui::CollapsingHeader("Plane Settings")) {
                    ImGui::PushItemWidth(150.0);
                    ImGui::SliderInt("Plane bounds", (int*)&app_state.plane_settings.bounds, 1, 100);
                    ImGui::SliderInt("Plane detail level", (int*)&app_state.plane_settings.detail, 1, 1000);
                    ImGui::SliderFloat("Plane grid step", &app_state.plane_settings.grid_step, 1.0f, 100.0f);
                    ImGui::SliderFloat("Plane grid line thickness", &app_state.plane_settings.grid_line_thickness, 0.1f, 5.0f);
                    ImGui::Checkbox("Plane detail affects grid step", &app_state.plane_settings.is_detail_affecting_grid_step);
                    ImGui::PopItemWidth();
                }
                if(ImGui::CollapsingHeader("Rendering Settings")) {
                   ImGui::Checkbox("Grid drawing", &app_state.render_settings.is_grid_rendered);
                   ImGui::Checkbox("Plane grid drawing", &app_state.render_settings.is_plane_grid_rendered);
                }
                break; 
            }
        }

    }
    ImGui::EndChild();   
}
static void draw_user_variables_table(SelectedTab tab) {
    static const auto draw_table = [](auto& data, auto draw_callback) {
        const auto NUM_COLS    = 2;
        const auto TABLE_FLAGS = ImGuiTableFlags_BordersH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
        const auto TABLE_SIZE = ImGui::GetContentRegionAvail();

        if (ImGui::BeginTable("2ColListDT", NUM_COLS, TABLE_FLAGS, TABLE_SIZE)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupScrollFreeze(NUM_COLS, 1);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < data.size(); ++i) {
                ImGui::PushID(i + 1);
                ImGui::TableNextRow();
                draw_callback(i, data.at(i), data);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    };

    static const auto EditableSelectable = [](size_t unique_idx, std::string &data, float overrided_height = 0.0f, bool disable_editing = false) {
        static std::map<size_t, std::pair<bool, bool>> data_specific_settings;
        auto &data_setting = data_specific_settings[unique_idx];
        bool &is_editing = data_setting.first;
        bool &focus_on_text_input = data_setting.second;

        if(is_editing == false) {
            ImGui::Selectable(data.c_str(), false, 0, ImVec2(ImGui::GetContentRegionAvail().x, overrided_height != 0.0f ? overrided_height : ImGui::GetTextLineHeightWithSpacing()));
            if(disable_editing == false && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) { is_editing = true; focus_on_text_input = true; }
        } else if(disable_editing == false) {
            if(focus_on_text_input) { focus_on_text_input = false; ImGui::SetKeyboardFocusHere(); }
            ImGui::InputText("##text", &data);
            if(ImGui::IsItemDeactivated()) { is_editing = false; app_state.needs_recompilation = true; }
        }

    };

    struct ItemState {
        bool mouse_hovering{false};
        bool mouse_just_stopped_hovering{false};
        bool mouse_clicked{false};
        bool mouse_double_clicked{false};
    };
    static const auto query_previous_item_state = [](ItemState& state) {
        bool is_hovering = ImGui::IsItemHovered();
        if(state.mouse_hovering && is_hovering == false) { state.mouse_just_stopped_hovering = true; }
        else if(state.mouse_just_stopped_hovering)       { state.mouse_just_stopped_hovering = false; }
        state.mouse_hovering        = is_hovering;
        state.mouse_clicked         = ImGui::IsMouseClicked(0);
        state.mouse_double_clicked  = ImGui::IsMouseDoubleClicked(0);
    };

    
    static const auto draw_function = [&](size_t idx, auto& data, auto &data_vector){
        static int swap_a=-1, swap_b=-1;
        static int draging_idx = -1;
        static std::map<size_t, ItemState> states;
        ItemState &s1 = states[idx*2+0], &s2 = states[idx*2+1];

        ImGui::TableSetColumnIndex(0);
        float overrided_height = ImGui::GetTextLineHeightWithSpacing()*1.0f;
        bool disable_editing   = false;
        if      constexpr(std::same_as<std::remove_cvref_t<decltype(data)>, Slider>)    { overrided_height = 3.0f*ImGui::GetTextLineHeightWithSpacing(); }
        else if constexpr(std::same_as<std::remove_cvref_t<decltype(data)>, Constant>)  { overrided_height = 1.1f*ImGui::GetTextLineHeightWithSpacing(); }
        if(data.name == "f" && std::same_as<std::remove_cvref_t<decltype(data)>, Function>) { disable_editing = true; } //F function cannot be renamed by the user.
        EditableSelectable(idx*2+0, data.name, overrided_height, disable_editing);
        query_previous_item_state(s1);

        ImGui::TableSetColumnIndex(1);
        if constexpr(std::same_as<std::remove_cvref_t<decltype(data)>, Function>) {
            EditableSelectable(idx*2+1, data.value);
        } 
        else if constexpr(std::same_as<std::remove_cvref_t<decltype(data)>, Constant>) {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if(ImGui::InputFloat("##const_value", &data.value, 0,0, "%.6f")) { app_state.needs_recompilation = true; }
        }
        else if constexpr(std::same_as<std::remove_cvref_t<decltype(data)>, Slider>) {
            Slider &s = static_cast<Slider&>(data);
            ImGui::PushItemWidth(45.0f);
            ImGui::InputFloat("Min", &s.min); ImGui::SameLine(); ImGui::InputFloat("Max", &s.max);
            ImGui::PushItemWidth(90.0f + 2.0f * ImGui::CalcTextSize("Min").x + ImGui::GetStyle().ItemSpacing.x*2.0f);
            if(ImGui::SliderFloat("##value", &s.value, s.min, s.max, "%.2f")) { app_state.needs_recompilation = true; }
            ImGui::PopItemWidth();
            ImGui::PopItemWidth();
        }
        query_previous_item_state(s2);

        bool just_stopped_hovering_any = s1.mouse_just_stopped_hovering;
        bool hovering_any = s1.mouse_hovering | s2.mouse_hovering;

        if(draging_idx == -1 && hovering_any && ImGui::IsMouseDragging(0, 0.1)) { draging_idx = static_cast<int>(idx); }

        if(swap_a == -1 && (int)idx == draging_idx && just_stopped_hovering_any && ImGui::IsMouseDragging(0, 0.0f)) {
            std::cout << ImGui::GetMouseDragDelta(0, 0.0f).y << ' ' << draging_idx << '\n';
            int delta = ImGui::GetMouseDragDelta(0, 0.0f).y > 0.0f ? 1 : ImGui::GetMouseDragDelta(0, 0.0f).y < 0.0f ? -1 : 0;
            if(delta == 0) { return; }
            swap_a = idx;
            swap_b = std::clamp((int)(idx + delta), (int)0, (int)data_vector.size()-1);
            ImGui::ResetMouseDragDelta(0);
        }

        if(ImGui::IsMouseReleased(0)) { draging_idx = swap_a = swap_b = -1; }
        if(idx == 0 && swap_a > -1 && swap_b > -1) {
            auto temp = data_vector.at(swap_a);
            data_vector.at(swap_a) = data_vector.at(swap_b);
            data_vector.at(swap_b) = temp;
            draging_idx = swap_b; swap_a = swap_b = -1;
        }
    };

    switch (tab) {
        case SelectedTab::Functions: { draw_table(app_state.functions, draw_function); break; }
        case SelectedTab::Constants: { draw_table(app_state.constants, draw_function); break; }
        case SelectedTab::Sliders:   { draw_table(app_state.sliders,   draw_function); break; }
        default: { assert(false && "Unexpected tab."); }
    }
}

static void save_project(const char *file_name) {
    std::ofstream file;
    file.open(file_name, std::ofstream::out);
    if (file.is_open() == false) { std::cerr << "File is not opened!"; return; }
    
    std::stringstream content;
    content << "[Functions]\n"; for (const auto &f : app_state.functions) { content << f.name << ' ' << f.value << '\n'; }
    content << "[Constants]\n"; for (const auto &f : app_state.constants) { content << f.name << ' ' << f.value << '\n'; }
    content << "[Sliders]\n";   for (const auto &f : app_state.sliders)   { content << f.name << ' ' << f.value << ' ' << f.min << ' ' << f.max << '\n'; }

    content << "[Color Settings]\n";
    /*Background color*/    for(int i=0; i<4; ++i) {content << app_state.color_settings.color_background[i]; if(i<3)content<<' ';} content << '\n';
    /*Grid color*/          for(int i=0; i<4; ++i) {content << app_state.color_settings.color_grid[i];       if(i<3)content<<' ';} content << '\n';
    /*Plane color*/         for(int i=0; i<4; ++i) {content << app_state.color_settings.color_plane[i];      if(i<3)content<<' ';} content << '\n';
    /*Plane Grid color*/    for(int i=0; i<4; ++i) {content << app_state.color_settings.color_plane_grid[i]; if(i<3)content<<' ';} content << '\n';

    content << "[Plane Settings]\n";
    auto &ps = app_state.plane_settings;
    /*Plane settings*/ content << ps.bounds << ' ' << ps.detail << ' ' << ps.grid_step << ' ' << ps.grid_line_thickness << ' ' << ps.is_detail_affecting_grid_step << '\n';

    content << "[Render Settings]\n";
    auto &rs = app_state.render_settings;
    content << rs.is_grid_rendered << ' ' << rs.is_plane_grid_rendered;

    file << content.rdbuf(); file.close();
}

static void load_project(const char *file_name) {
    std::ifstream file{file_name};
    if (file.is_open() == false) { throw std::runtime_error{"File could not be opened"}; }

    std::string line;
    int resource_id = -1;
    app_state.functions.clear();
    app_state.constants.clear();
    app_state.sliders.clear();
    while (std::getline(file, line)) {
        if      (line.starts_with("[Functions]"))           { resource_id = 0; continue; }
        else if (line.starts_with("[Constants]"))           { resource_id = 1; continue; }
        else if (line.starts_with("[Sliders]"))             { resource_id = 2; continue; }
        else if (line.starts_with("[Color Settings]"))      { resource_id = 3; continue; }
        else if (line.starts_with("[Plane Settings]"))      { resource_id = 4; continue; }
        else if (line.starts_with("[Render Settings]"))     { resource_id = 5; continue; }

        if (resource_id == -1 || resource_id > 5) { throw std::runtime_error{"Error while reading the project file - label not found: " + line}; }

        std::stringstream ssline{line};
        switch (resource_id) {
        case 0: {
            Function f;
            ssline >> f.name >> f.value;
            while (ssline.eof()==false) { std::string rest; ssline >> rest; f.value += rest; }
            app_state.functions.push_back(f);
            break;
        }
        case 1: {
            Constant c;
            ssline >> c.name >> c.value;
            app_state.constants.push_back(c);
            break;
        }
        case 2: {
            Slider s;
            ssline >> s.name >> s.value >> s.min >> s.max;
            app_state.sliders.push_back(s);
            break;
        }
        case 3: {
            auto &cs = app_state.color_settings;
            ssline >> cs.color_background[0] >> cs.color_background[1] >> cs.color_background[2] >> cs.color_background[3]; std::getline(file, line); ssline = std::stringstream{line};    
            ssline >> cs.color_grid[0] >> cs.color_grid[1] >> cs.color_grid[2] >> cs.color_grid[3];                         std::getline(file, line); ssline = std::stringstream{line};    
            ssline >> cs.color_plane[0] >> cs.color_plane[1] >> cs.color_plane[2] >> cs.color_plane[3];                     std::getline(file, line); ssline = std::stringstream{line};        
            ssline >> cs.color_plane_grid[0] >> cs.color_plane_grid[1] >> cs.color_plane_grid[2] >> cs.color_plane_grid[3];
            break;
        }
        case 4: {
            auto &ps = app_state.plane_settings;
            ssline >> ps.bounds >> ps.detail >> ps.grid_step >> ps.grid_line_thickness >> ps.is_detail_affecting_grid_step;
            break;
        }
        case 5: {
            auto &rs = app_state.render_settings;
            ssline >> rs.is_grid_rendered >> rs.is_plane_grid_rendered;
            break;
        }
        }
    }
}
static void export_to_obj(const std::string& path, g3d::HandleBuffer buffer) {
    const auto buffer_size = (app_state.plane_settings.detail + 1) * (app_state.plane_settings.detail + 1);
    const auto *vs = (float*)glMapNamedBufferRange(buffer, 0, buffer_size, GL_MAP_READ_BIT);

    std::string file_content;

    const auto s = app_state.plane_settings.bounds;
    const auto n = app_state.plane_settings.detail;
    for(auto i=0llu; i<buffer_size; ++i) {
        float x = i % (n+1);
        float z = i / (n+1); // x,z are in [0, detail] range
        float y = vs[(uint32_t)z*(n+1) + (uint32_t)x];
        x /= n; z /= n;  // x,z are in [0, 1] range
        x = x*2.0*s - s;
        z = z*2.0*s - s; // x,z are in [-bounds, bounds] range

        file_content +=  "v " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + '\n';
    }

    file_content += '\n';

    for(auto i=0llu; i<n; ++i) {
        for(auto j=0llu; j<n; ++j) {
            uint32_t idx = i * (n+1) + j;
            uint32_t a = idx;
            uint32_t b = idx + (n+1);
            uint32_t c = idx + 1;
            uint32_t d = b + 1;

            ++a; ++b; ++c; ++d;

            file_content += "f " + std::to_string(a) + ' ' + std::to_string(b) + ' ' + std::to_string(c) + '\n';
            file_content += "f " + std::to_string(c) + ' ' + std::to_string(b) + ' ' + std::to_string(d) + '\n';
        }
    }
    glUnmapNamedBuffer(buffer);

    std::fstream file{path, std::ios::trunc | std::ios::out};
    file << file_content;

}

static void log_list_add_message(const std::string& msg) {
    app_state.logs.push_back(msg);
    app_state.log_list_scroll_down = true;
}

static void uniform1f(uint32_t program, const char* name, float value) {
    const auto location = glGetUniformLocation(program, name);
    glUniform1f(location, value);
}

static void uniform3f(uint32_t program, const char* name, glm::vec3 value) {
    const auto location = glGetUniformLocation(program, name);
    glUniform3fv(location, 1, &value.x);
}

static void uniformm4(uint32_t program, const char* name, const glm::mat4& value) {
    const auto location = glGetUniformLocation(program, name);
    glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
}