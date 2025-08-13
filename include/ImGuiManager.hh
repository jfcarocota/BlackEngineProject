#pragma once

#include "SFML/Graphics.hpp"
#include "imgui.h"

class ImGuiManager {
 private:
  bool m_showDemoWindow = false;
  bool m_showDebugInfo = false;
  bool m_showEntityInfo = false;
  bool m_showTestWindow = true;  // Show by default for testing
  bool m_initialized = false;

 public:
  ImGuiManager();
  ~ImGuiManager();

  void Initialize(sf::RenderWindow& window);
  void ProcessEvent(const sf::Event& event);
  void Update(sf::RenderWindow& window, sf::Time deltaTime);
  void Render(sf::RenderWindow& window);
  void Shutdown();

  // Debug UI functions
  void ShowMainMenuBar();
  void ShowDemoWindow();
  void ShowDebugInfo();
  void ShowEntityInfo();
  void ShowTestWindow();
  void RenderTestIndicator(sf::RenderWindow& window);

  // Getters for UI state
  bool IsDemoWindowVisible() const { return m_showDemoWindow; }
  bool IsDebugInfoVisible() const { return m_showDebugInfo; }
  bool IsEntityInfoVisible() const { return m_showEntityInfo; }
  bool IsTestWindowVisible() const { return m_showTestWindow; }
  bool IsInitialized() const { return m_initialized; }
};
