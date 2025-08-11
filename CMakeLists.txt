cmake_minimum_required(VERSION 3.21)
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

# Tile Map Editor tool
add_executable(TileMapEditor
  src/MapEditorMain.cpp
)

# Dependencies (cross-platform)
# Vendor SFML 3 (drops OpenAL requirement; uses miniaudio internally)
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# Ensure ImGui sources exist locally (fallback to fetch into third_party/imgui)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
if(NOT EXISTS ${IMGUI_DIR}/imgui.cpp)
  message(STATUS "ImGui no encontrado en third_party/imgui, descargando...")
  # Reuse FetchContent to populate directly into third_party/imgui
  include(FetchContent)
  FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.8
    GIT_SHALLOW ON
    SOURCE_DIR ${IMGUI_DIR}
  )
  FetchContent_Populate(imgui)
endif()

# Prefer config packages; fall back to FetchContent so it works on Windows/macOS/Linux
# Box2D
find_package(box2d CONFIG QUIET)
if(NOT box2d_FOUND)
  FetchContent_Declare(box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v2.4.1
    GIT_SHALLOW ON)
  # Disable Box2D extras to speed up Windows/Linux builds
  set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
  set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
  set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(box2d)
  set(BOX2D_TARGET box2d)
else()
  set(BOX2D_TARGET box2d::box2d)
endif()

# JsonCpp
find_package(JsonCpp CONFIG QUIET)
if(JsonCpp_FOUND)
  set(JSONCPP_TARGET JsonCpp::JsonCpp)
else()
  FetchContent_Declare(jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG 1.9.5
    GIT_SHALLOW ON)
  # Disable extras to speed up
  set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
  set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
  set(JSONCPP_WITH_EXAMPLE OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(jsoncpp)
  # Elegir el target correcto según lo que exporte jsoncpp
  if(TARGET jsoncpp_static)
    set(JSONCPP_TARGET jsoncpp_static)
  elseif(TARGET jsoncpp_lib)
    set(JSONCPP_TARGET jsoncpp_lib)
  elseif(TARGET JsonCpp::JsonCpp)
    set(JSONCPP_TARGET JsonCpp::JsonCpp)
  else()
    message(FATAL_ERROR "No se encontró el target de JsonCpp tras FetchContent")
  endif()
  # Asegurar rutas de cabeceras cuando usamos FetchContent (por si el target no las propaga)
  target_include_directories(BlackEngineProject PRIVATE ${jsoncpp_SOURCE_DIR}/include ${jsoncpp_BINARY_DIR}/include)
endif()

# ImGui include path
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)

target_include_directories(BlackEngineProject PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${IMGUI_DIR}
)

target_include_directories(TileMapEditor PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${IMGUI_DIR}
)

target_link_libraries(BlackEngineProject PRIVATE
  sfml-graphics
  sfml-window
  sfml-system
  sfml-audio
  ${BOX2D_TARGET}
  ${JSONCPP_TARGET}
)

target_link_libraries(TileMapEditor PRIVATE
  sfml-graphics
  sfml-window
  sfml-system
  sfml-audio
)

# Enable native Windows file dialogs for the editor
if(WIN32)
  target_compile_definitions(TileMapEditor PRIVATE MAPEDITOR_ENABLE_WIN32_DIALOGS=1)
  # Link common Windows system libs for COM and dialogs (pragma should handle MSVC, this is explicit)
  target_link_libraries(TileMapEditor PRIVATE ole32 shell32 comdlg32)
endif()

# Enable SFML audio where used
target_compile_definitions(BlackEngineProject PRIVATE SFML_AUDIO_AVAILABLE)

# On Windows, copy dependent DLLs next to the executables for easy run
if(WIN32)
  set(COPY_DLL_SCRIPT ${CMAKE_SOURCE_DIR}/cmake/copy_runtime_dlls.cmake)
  add_custom_command(TARGET BlackEngineProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -Ddlls="$<TARGET_RUNTIME_DLLS:BlackEngineProject>" -Ddest="$<TARGET_FILE_DIR:BlackEngineProject>" -P ${COPY_DLL_SCRIPT}
    COMMAND_EXPAND_LISTS)
  add_custom_command(TARGET TileMapEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -Ddlls="$<TARGET_RUNTIME_DLLS:TileMapEditor>" -Ddest="$<TARGET_FILE_DIR:TileMapEditor>" -P ${COPY_DLL_SCRIPT}
    COMMAND_EXPAND_LISTS)
endif()

# On macOS, set rpath and bundle metadata
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
  set_target_properties(BlackEngineProject PROPERTIES
    INSTALL_RPATH "@executable_path;@executable_path/../Frameworks"
  )
  # Apply same rpath for the editor
  set_target_properties(TileMapEditor PROPERTIES
    INSTALL_RPATH "@executable_path;@executable_path/../Frameworks"
  )

  # Bundle metadata / Info.plist for the main app
  set(BUNDLE_IDENTIFIER "com.blackengine.project")
  set(BUNDLE_ICON_FILE "AppIcon.icns")
  set(BUNDLE_ICON_SOURCE ${CMAKE_SOURCE_DIR}/assets/GUI/AppIcon.icns)
  if(NOT EXISTS ${BUNDLE_ICON_SOURCE})
    set(BUNDLE_ICON_FILE "")
  endif()

  # Ensure PRODUCT_NAME matches the executable we place in Contents/MacOS
  set(PRODUCT_NAME "BlackEngineProject-bin")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
                 ${CMAKE_SOURCE_DIR}/BlackEngineProject.app/Contents/Info.plist
                 @ONLY)

  # Editor bundle metadata (separate identifier and product name)
  set(BUNDLE_IDENTIFIER "com.blackengine.tilemapeditor")
  set(PRODUCT_NAME "TileMapEditor-bin")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
                 ${CMAKE_SOURCE_DIR}/TileMapEditor.app/Contents/Info.plist
                 @ONLY)
endif()

# Copy assets to build directory for standalone executable
add_custom_command(TARGET BlackEngineProject POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/assets
  ${CMAKE_BINARY_DIR}/assets
)

# Also copy assets next to the binary (useful for multi-config generators like MSVC)
add_custom_command(TARGET BlackEngineProject POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:BlackEngineProject>/assets
)

# Copy assets so editor can run from build dir
add_custom_command(TARGET TileMapEditor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/assets
  ${CMAKE_BINARY_DIR}/assets
)

# Also copy assets next to the editor binary
add_custom_command(TARGET TileMapEditor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:TileMapEditor>/assets
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

  # Create a bundle for the Tile Map Editor as well
  add_custom_command(TARGET TileMapEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_SOURCE_DIR}/TileMapEditor.app/Contents/MacOS
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_SOURCE_DIR}/TileMapEditor.app/Contents/Resources

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:TileMapEditor>
            ${CMAKE_SOURCE_DIR}/TileMapEditor.app/Contents/MacOS/TileMapEditor-bin

    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_SOURCE_DIR}/TileMapEditor.app/Contents/Resources/assets
  )

  if(EXISTS ${BUNDLE_ICON_SOURCE})
    add_custom_command(TARGET TileMapEditor POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${BUNDLE_ICON_SOURCE}
              ${CMAKE_SOURCE_DIR}/TileMapEditor.app/Contents/Resources/${BUNDLE_ICON_FILE}
    )
  endif()
endif()

# Install/packaging target to create dist/.app bundle copy (macOS only)
if(APPLE)
  set(DIST_DIR ${CMAKE_SOURCE_DIR}/dist)
  add_custom_target(package_app ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/BlackEngineProject.app
            ${DIST_DIR}/BlackEngineProject.app
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/TileMapEditor.app
            ${DIST_DIR}/TileMapEditor.app
    COMMENT "Packaging .app into dist/"
  )
  add_dependencies(package_app BlackEngineProject)
  add_dependencies(package_app TileMapEditor)
endif()

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

# Install rules for packaging (Windows/Linux): put executables in bin/ and assets at root
install(TARGETS BlackEngineProject RUNTIME DESTINATION bin)
install(TARGETS TileMapEditor RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION .)

# CPack configuration to produce a ZIP
set(CPACK_PACKAGE_NAME "BlackEngineProject")
set(CPACK_PACKAGE_VENDOR "BlackEngine")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP")
# Name like BlackEngineProject-1.0.0-Windows-x86_64.zip
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)

