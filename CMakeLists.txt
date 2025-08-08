cmake_minimum_required(VERSION 3.16)
project(BlackEngineProject CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(BlackEngineProject
  src/main.cpp
  src/Components/AnimatorComponent.cc
  src/Components/AudioListenerComponent.cc
  src/Components/Entity.cc
  src/Components/EntityManager.cc
  src/Components/RigidBodyComponent.cc
  src/Components/SpriteComponent.cc
  src/Components/TransformComponent.cc
  src/Animation.cc
  src/AudioClip.cc
  src/ContactEventManager.cc
  src/DrawPhysics.cc
  src/FlipSprite.cc
  src/Game.cc
  src/ImGuiManager.cc
  src/Movement.cc
  src/Tile.cc
  src/TileGroup.cc
  src/GUI/Button.cc
  src/GUI/TextObject.cc
  third_party/imgui/imgui.cpp
  third_party/imgui/imgui_demo.cpp
  third_party/imgui/imgui_draw.cpp
  third_party/imgui/imgui_tables.cpp
  third_party/imgui/imgui_widgets.cpp
)

# Dependencies (use pkg-config for better compatibility)
find_package(PkgConfig REQUIRED)

# SFML via pkg-config (Homebrew version) - excluding audio for now due to OpenAL issues
pkg_check_modules(SFML REQUIRED sfml-system sfml-window sfml-graphics)
# Try to find audio separately (optional)
pkg_check_modules(SFML_AUDIO sfml-audio)

# Box2D via CMake
find_package(box2d REQUIRED)

# JsonCpp via pkg-config
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# ImGui-SFML integration (direct compilation)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)

target_include_directories(BlackEngineProject PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${SFML_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
  ${IMGUI_DIR}
)

target_link_libraries(BlackEngineProject PRIVATE
  ${SFML_LIBRARIES}
  box2d::box2d
  ${JSONCPP_LIBRARIES}
)

# Add audio if available
if(SFML_AUDIO_FOUND)
  target_link_libraries(BlackEngineProject PRIVATE ${SFML_AUDIO_LIBRARIES})
  target_compile_definitions(BlackEngineProject PRIVATE SFML_AUDIO_AVAILABLE)
endif()

# On macOS, ensure rpath is used so the executable can find Homebrew-installed dylibs
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

# Copy assets to build directory for standalone executable
add_custom_command(TARGET BlackEngineProject POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/assets
  ${CMAKE_BINARY_DIR}/assets
)