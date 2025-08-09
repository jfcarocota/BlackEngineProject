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

    if (event.is<sf::Event::MouseMoved>()) {
        if (const auto* e = event.getIf<sf::Event::MouseMoved>()) {
            io.MousePos.x = static_cast<float>(e->position.x);
            io.MousePos.y = static_cast<float>(e->position.y);
        }
        return;
    }
    if (event.is<sf::Event::MouseButtonPressed>()) {
        if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (e->button == sf::Mouse::Button::Left) io.MouseDown[0] = true;
            if (e->button == sf::Mouse::Button::Right) io.MouseDown[1] = true;
            if (e->button == sf::Mouse::Button::Middle) io.MouseDown[2] = true;
        }
        return;
    }
    if (event.is<sf::Event::MouseButtonReleased>()) {
        if (const auto* e = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (e->button == sf::Mouse::Button::Left) io.MouseDown[0] = false;
            if (e->button == sf::Mouse::Button::Right) io.MouseDown[1] = false;
            if (e->button == sf::Mouse::Button::Middle) io.MouseDown[2] = false;
        }
        return;
    }
    if (event.is<sf::Event::MouseWheelScrolled>()) {
        if (const auto* e = event.getIf<sf::Event::MouseWheelScrolled>()) {
            io.MouseWheel += e->delta;
        }
        return;
    }
    if (event.is<sf::Event::KeyPressed>()) {
        if (const auto* e = event.getIf<sf::Event::KeyPressed>()) {
            if (e->code == sf::Keyboard::Key::F3) {
                m_showTestWindow = !m_showTestWindow;
                std::cout << "ImGui Test Window: " << (m_showTestWindow ? "ON" : "OFF") << std::endl;
            }
            if (e->code == sf::Keyboard::Key::Escape) {
                m_showTestWindow = false;
                std::cout << "ImGui Test Window: OFF" << std::endl;
            }
        }
        return;
    }
    if (event.is<sf::Event::TextEntered>()) {
        if (const auto* e = event.getIf<sf::Event::TextEntered>()) {
            if (e->unicode > 0 && e->unicode < 0x10000)
                io.AddInputCharacter(static_cast<unsigned short>(e->unicode));
        }
        return;
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
    
    if (m_showTestWindow) {
        ShowTestWindow();
    }
    */
}

void ImGuiManager::Render(sf::RenderWindow& window) {
    if (!m_initialized) return;
    
    // For now, we'll skip ImGui rendering to avoid crashes
    // In a full implementation, you would need to set up OpenGL rendering
    // or use a proper ImGui backend like ImGui-SFML
    
    // ImGui::Render();
    
    // Render a simple test indicator when test window is enabled
    if (m_showTestWindow) {
        RenderTestIndicator(window);
    }
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
            ImGui::MenuItem("Test Window", nullptr, &m_showTestWindow);
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

void ImGuiManager::RenderTestIndicator(sf::RenderWindow& window) {
    // Create a simple test indicator using SFML
    sf::RectangleShape testRect(sf::Vector2f(400, 300));
    testRect.setPosition(sf::Vector2f(50, 50));
    testRect.setFillColor(sf::Color(0, 100, 200, 200)); // Semi-transparent blue
    testRect.setOutlineColor(sf::Color::Yellow);
    testRect.setOutlineThickness(3);
    
    window.draw(testRect);
    
    // Add some text to show it's working
    static sf::Font font;
    static bool fontLoaded = false;
    
    if (!fontLoaded) {
        if (font.openFromFile("assets/fonts/ARCADECLASSIC.TTF")) {
            fontLoaded = true;
        }
    }
    
    if (fontLoaded) {
        sf::Text text(font);
        text.setString("ImGui Test Window Active!\n\nPress F3 to toggle\nPress ESC to close\n\nThis is a test UI indicator\nshowing ImGui integration is working!");
        text.setCharacterSize(18);
        text.setFillColor(sf::Color::White);
        text.setPosition(sf::Vector2f(70, 70));
        
        window.draw(text);
    }
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

void ImGuiManager::ShowTestWindow() {
    ImGui::Begin("ImGui Test Window", &m_showTestWindow);
    
    ImGui::Text("Welcome to ImGui Test Window!");
    ImGui::Separator();
    
    // Test different ImGui widgets
    static float sliderValue = 0.5f;
    ImGui::SliderFloat("Test Slider", &sliderValue, 0.0f, 1.0f);
    
    static int comboSelection = 0;
    const char* items[] = { "Option 1", "Option 2", "Option 3", "Option 4" };
    ImGui::Combo("Test Combo", &comboSelection, items, IM_ARRAYSIZE(items));
    
    static bool checkbox1 = false;
    static bool checkbox2 = true;
    ImGui::Checkbox("Test Checkbox 1", &checkbox1);
    ImGui::Checkbox("Test Checkbox 2", &checkbox2);
    
    static char textInput[128] = "Hello ImGui!";
    ImGui::InputText("Test Input", textInput, IM_ARRAYSIZE(textInput));
    
    ImGui::Separator();
    
    if (ImGui::Button("Test Button")) {
        ImGui::OpenPopup("Test Popup");
    }
    
    if (ImGui::BeginPopup("Test Popup")) {
        ImGui::Text("This is a test popup!");
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    
    ImGui::Text("Current Values:");
    ImGui::Text("Slider: %.3f", sliderValue);
    ImGui::Text("Combo: %s", items[comboSelection]);
    ImGui::Text("Checkbox 1: %s", checkbox1 ? "ON" : "OFF");
    ImGui::Text("Checkbox 2: %s", checkbox2 ? "ON" : "OFF");
    ImGui::Text("Input: %s", textInput);
    
    ImGui::End();
}
