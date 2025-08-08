#include "ImGuiManager.hh"
#include <iostream>

ImGuiManager::ImGuiManager() {
    // Constructor
}

ImGuiManager::~ImGuiManager() {
    Shutdown();
}

void ImGuiManager::Initialize(sf::RenderWindow& window) {
    if (m_initialized) {
        return;
    }
    
    // Initialize ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Set up style
    ImGui::StyleColorsDark();
    
    m_initialized = true;
    std::cout << "ImGui initialized successfully" << std::endl;
}

void ImGuiManager::ProcessEvent(const sf::Event& event) {
    if (!m_initialized) return;
    
    ImGuiIO& io = ImGui::GetIO();
    
    switch (event.type) {
        case sf::Event::MouseMoved:
            io.MousePos.x = static_cast<float>(event.mouseMove.x);
            io.MousePos.y = static_cast<float>(event.mouseMove.y);
            break;
        case sf::Event::MouseButtonPressed:
            if (event.mouseButton.button == sf::Mouse::Left) io.MouseDown[0] = true;
            if (event.mouseButton.button == sf::Mouse::Right) io.MouseDown[1] = true;
            if (event.mouseButton.button == sf::Mouse::Middle) io.MouseDown[2] = true;
            break;
        case sf::Event::MouseButtonReleased:
            if (event.mouseButton.button == sf::Mouse::Left) io.MouseDown[0] = false;
            if (event.mouseButton.button == sf::Mouse::Right) io.MouseDown[1] = false;
            if (event.mouseButton.button == sf::Mouse::Middle) io.MouseDown[2] = false;
            break;
        case sf::Event::MouseWheelScrolled:
            io.MouseWheel += event.mouseWheelScroll.delta;
            break;
        case sf::Event::KeyPressed:
            // Handle key input manually for now
            break;
        case sf::Event::KeyReleased:
            // Handle key input manually for now
            break;
        case sf::Event::TextEntered:
            if (event.text.unicode > 0 && event.text.unicode < 0x10000)
                io.AddInputCharacter(static_cast<unsigned short>(event.text.unicode));
            break;
    }
}

void ImGuiManager::Update(sf::RenderWindow& window, sf::Time deltaTime) {
    if (!m_initialized) return;
    
    // For now, we'll skip ImGui frame updates to avoid rendering issues
    // In a full implementation, you would need to set up proper ImGui rendering
    
    /*
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = deltaTime.asSeconds();
    
    // Update display size
    sf::Vector2u size = window.getSize();
    io.DisplaySize.x = static_cast<float>(size.x);
    io.DisplaySize.y = static_cast<float>(size.y);
    
    ImGui::NewFrame();
    
    // Show main menu bar
    ShowMainMenuBar();
    
    // Show windows based on state
    if (m_showDemoWindow) {
        ShowDemoWindow();
    }
    
    if (m_showDebugInfo) {
        ShowDebugInfo();
    }
    
    if (m_showEntityInfo) {
        ShowEntityInfo();
    }
    */
}

void ImGuiManager::Render(sf::RenderWindow& window) {
    if (!m_initialized) return;
    
    // For now, we'll skip ImGui rendering to avoid crashes
    // In a full implementation, you would need to set up OpenGL rendering
    // or use a proper ImGui backend like ImGui-SFML
    
    // ImGui::Render();
}

void ImGuiManager::Shutdown() {
    if (m_initialized) {
        ImGui::DestroyContext();
        m_initialized = false;
    }
}

void ImGuiManager::ShowMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Demo Window", nullptr, &m_showDemoWindow);
            ImGui::MenuItem("Debug Info", nullptr, &m_showDebugInfo);
            ImGui::MenuItem("Entity Info", nullptr, &m_showEntityInfo);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // Could show an about dialog
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void ImGuiManager::ShowDemoWindow() {
    ImGui::ShowDemoWindow(&m_showDemoWindow);
}

void ImGuiManager::ShowDebugInfo() {
    ImGui::Begin("Debug Info", &m_showDebugInfo);
    
    ImGui::Text("BlackEngine Debug Information");
    ImGui::Separator();
    
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Move player");
    ImGui::BulletText("Space - Jump");
    ImGui::BulletText("F1 - Toggle debug info");
    ImGui::BulletText("F2 - Toggle demo window");
    
    ImGui::End();
}

void ImGuiManager::ShowEntityInfo() {
    ImGui::Begin("Entity Information", &m_showEntityInfo);
    
    ImGui::Text("Entity System Information");
    ImGui::Separator();
    
    // This could be expanded to show actual entity data
    ImGui::Text("Entities: %d", 0); // Placeholder
    ImGui::Text("Active Components: %d", 0); // Placeholder
    
    ImGui::Separator();
    ImGui::Text("Component Types:");
    ImGui::BulletText("TransformComponent");
    ImGui::BulletText("SpriteComponent");
    ImGui::BulletText("RigidBodyComponent");
    ImGui::BulletText("AnimatorComponent");
    ImGui::BulletText("AudioListenerComponent");
    
    ImGui::End();
}
