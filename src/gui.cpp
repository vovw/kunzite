#include "gui.h"
#include "llm.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <mutex>
#include <vector>
#include <string>

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

struct ChatMessage {
    std::string sender;
    std::string content;
};

static std::vector<ChatMessage> chat_messages;
static std::mutex messages_mutex;
static bool is_waiting_response = false;
static std::string current_response;

void render_chat_window(GLFWwindow* window, LLM& llm) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Chat Window", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::BeginChild("ChatArea", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    {
        std::lock_guard<std::mutex> lock(messages_mutex);
        for (const auto& message : chat_messages) {
            ImGui::TextWrapped("%s: %s", message.sender.c_str(), message.content.c_str());
            ImGui::Spacing();
        }
        if (is_waiting_response) {
            ImGui::TextWrapped("AI: %s", current_response.c_str());
        }
    }
    ImGui::EndChild();

    static char input_buffer[256] = "";
    ImGui::PushItemWidth(-1);
    if (ImGui::InputText("##input", input_buffer, IM_ARRAYSIZE(input_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (input_buffer[0] != '\0' && !is_waiting_response) {
            std::string message(input_buffer);
            {
                std::lock_guard<std::mutex> lock(messages_mutex);
                chat_messages.push_back({"User", message});
            }
            input_buffer[0] = '\0';
            is_waiting_response = true;
            current_response.clear();
            llm.send_message(message);
        }
    }
    ImGui::PopItemWidth();

    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

int run_gui(LLM& llm) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "kunzite", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    llm.on_token_received = [](const std::string& token) {
        std::lock_guard<std::mutex> lock(messages_mutex);
        current_response += token;
    };

    llm.on_response_complete = [](const std::string& response) {
        std::lock_guard<std::mutex> lock(messages_mutex);
        chat_messages.push_back({"AI", response});
        is_waiting_response = false;
        current_response.clear();
    };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        render_chat_window(window, llm);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
