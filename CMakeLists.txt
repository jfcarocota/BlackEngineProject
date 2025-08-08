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

# Dependencies (use pkg-config for better compatibility)
find_package(PkgConfig REQUIRED)

# SFML via pkg-config (Homebrew version)
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
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
  # Search both next to executable and in app Frameworks
  set_target_properties(BlackEngineProject PROPERTIES
    INSTALL_RPATH "@executable_path;@executable_path/../Frameworks"
  )

  # Bundle metadata
  set(BUNDLE_IDENTIFIER "com.blackengine.project")
  set(BUNDLE_ICON_FILE "AppIcon.icns")
  set(BUNDLE_ICON_SOURCE ${CMAKE_SOURCE_DIR}/assets/GUI/AppIcon.icns)
  # If icon is missing, clear the icon file so Info.plist doesn't reference it
  if(NOT EXISTS ${BUNDLE_ICON_SOURCE})
    set(BUNDLE_ICON_FILE "")
  endif()

  # Configure Info.plist
  configure_file(${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
                 ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Info.plist
                 @ONLY)

  # Try to locate an OpenAL dylib from common prefixes
  set(OPENAL_CANDIDATES
    /usr/local/lib/libopenal.1.dylib
    /opt/homebrew/lib/libopenal.1.dylib
  )
  set(OPENAL_DYLIB "")
  foreach(p ${OPENAL_CANDIDATES})
    if(EXISTS ${p})
      set(OPENAL_DYLIB ${p})
      break()
    endif()
  endforeach()

  # Bundle OpenAL inside the app bundle if found
  if(OPENAL_DYLIB)
    add_custom_command(TARGET BlackEngineProject POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Frameworks
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${OPENAL_DYLIB}
              ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Frameworks/libopenal.1.dylib
    )
  endif()
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
    # Ensure destination dirs exist
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/MacOS
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Resources

    # Copy built binary into the app bundle (as BlackEngineProject-bin to keep script as launcher)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:BlackEngineProject>
            ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/MacOS/BlackEngineProject-bin

    # Copy assets into app Resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Resources/assets
  )

  # Copy icon only if present
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