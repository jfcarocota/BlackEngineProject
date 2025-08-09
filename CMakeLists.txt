cmake_minimum_required(VERSION 3.16)
project(BlackEngineProject CXX)
set(PROJECT_VERSION 1.0.0)

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

# Dependencies
find_package(PkgConfig REQUIRED)

# Vendor SFML 3 (drops OpenAL requirement; uses miniaudio internally)
include(FetchContent)
FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.1
)
FetchContent_MakeAvailable(SFML)

# Box2D via CMake
find_package(box2d REQUIRED)

# JsonCpp via pkg-config
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# ImGui include path
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)

target_include_directories(BlackEngineProject PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${JSONCPP_INCLUDE_DIRS}
  ${IMGUI_DIR}
)

target_link_libraries(BlackEngineProject PRIVATE
  sfml-graphics
  sfml-window
  sfml-system
  sfml-audio
  box2d::box2d
  ${JSONCPP_LIBRARIES}
)

target_compile_definitions(BlackEngineProject PRIVATE SFML_AUDIO_AVAILABLE)

# On macOS, set rpath and bundle metadata
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
  set_target_properties(BlackEngineProject PROPERTIES
    INSTALL_RPATH "@executable_path;@executable_path/../Frameworks"
  )

  # Bundle metadata / Info.plist
  set(BUNDLE_IDENTIFIER "com.blackengine.project")
  set(BUNDLE_ICON_FILE "AppIcon.icns")
  set(BUNDLE_ICON_SOURCE ${CMAKE_SOURCE_DIR}/assets/GUI/AppIcon.icns)
  if(NOT EXISTS ${BUNDLE_ICON_SOURCE})
    set(BUNDLE_ICON_FILE "")
  endif()

  configure_file(${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
                 ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Info.plist
                 @ONLY)
endif()

# Copy assets to build directory for standalone executable
add_custom_command(TARGET BlackEngineProject POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/assets
  ${CMAKE_BINARY_DIR}/assets
)

# Package into .app bundle: copy binary and assets
if(APPLE)
  add_custom_command(TARGET BlackEngineProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/MacOS
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Resources

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:BlackEngineProject>
            ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/MacOS/BlackEngineProject-bin

    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Resources/assets
  )

  if(EXISTS ${BUNDLE_ICON_SOURCE})
    add_custom_command(TARGET BlackEngineProject POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${BUNDLE_ICON_SOURCE}
              ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Resources/${BUNDLE_ICON_FILE}
    )
  endif()
endif()

# Install/packaging target to create dist/.app bundle copy
set(DIST_DIR ${CMAKE_SOURCE_DIR}/dist)
add_custom_target(package_app ALL
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${DIST_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/BlackEngineProject.app
          ${DIST_DIR}/BlackEngineProject.app
  COMMENT "Packaging .app into dist/"
)
add_dependencies(package_app BlackEngineProject)

# Signing / notarization targets
option(MAC_SIGN "Code sign the app" OFF)
set(MAC_IDENTITY "" CACHE STRING "Codesign identity (e.g., Developer ID Application: Your Name (TEAMID))")
set(MAC_TEAM_ID "" CACHE STRING "Apple Developer Team ID")
set(MAC_NOTARIZE "OFF" CACHE STRING "Notarize the app (ON/OFF)")
set(APPLE_ID "" CACHE STRING "Apple ID email for notarization")
set(APPLE_ID_PASS "" CACHE STRING "App-specific password for notarization (or keychain reference)")
set(MAC_BUNDLE_PATH ${CMAKE_SOURCE_DIR}/BlackEngineProject.app)
set(MAC_ENTITLEMENTS ${CMAKE_SOURCE_DIR}/cmake/Entitlements.plist)

if(APPLE)
  add_custom_target(sign_app
    COMMAND /bin/sh -c "if [ \"${MAC_SIGN}\" = \"ON\" ] && [ -n \"${MAC_IDENTITY}\" ]; then \
      codesign --force --deep --options runtime --entitlements ${MAC_ENTITLEMENTS} --sign \"${MAC_IDENTITY}\" \"${MAC_BUNDLE_PATH}\"; \
    else \
      echo 'Skipping codesign: set -DMAC_SIGN=ON -DMAC_IDENTITY=...'; \
    fi"
    DEPENDS BlackEngineProject
    COMMENT "Code signing app bundle"
  )

  add_custom_target(notarize_app
    COMMAND /bin/sh -c "if [ \"${MAC_NOTARIZE}\" = \"ON\" ] && [ -n \"${APPLE_ID}\" ] && [ -n \"${APPLE_ID_PASS}\" ]; then \
      xcrun notarytool submit \"${MAC_BUNDLE_PATH}\" --apple-id \"${APPLE_ID}\" --password \"${APPLE_ID_PASS}\" --team-id \"${MAC_TEAM_ID}\" --wait; \
      xcrun stapler staple \"${MAC_BUNDLE_PATH}\"; \
    else \
      echo 'Skipping notarization: set -DMAC_NOTARIZE=ON and provide APPLE_ID, APPLE_ID_PASS, MAC_TEAM_ID'; \
    fi"
    DEPENDS sign_app
    COMMENT "Notarizing app bundle"
  )
endif()
# Microsoft GSL
include(FetchContent)
FetchContent_Declare(
  GSL
  GIT_REPOSITORY https://github.com/microsoft/GSL.git
  GIT_TAG v4.0.0
)
FetchContent_MakeAvailable(GSL)
target_link_libraries(BlackEngineProject PRIVATE Microsoft.GSL::GSL)

