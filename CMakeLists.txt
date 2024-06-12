cmake_minimum_required(VERSION 3.25)
project(BlackEngineProject)

set(CMAKE_CXX_STANDARD 23)

add_executable(BlackEngineProject src/main.cpp src/Components/AnimatorComponent.cc src/Components/AudioListenerComponent.cc src/Components/Entity.cc src/Components/EntityManager.cc src/Components/RigidBodyComponent.cc src/Components/SpriteComponent.cc src/Components/TransformComponent.cc src/Animation.cc src/AudioClip.cc src/ContactEventManager.cc src/DrawPhysics.cc src/FlipSprite.cc src/Game.cc src/Movement.cc src/Tile.cc src/TileGroup.cc src/GUI/Button.cc src/GUI/TextObject.cc)

set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_DIR F:/SFML-2.6.0-windows-gcc-13.1.0-mingw-32-bit/SFML-2.6.0/lib/cmake/SFML)

find_package(SFML COMPONENTS system window graphics audio network REQUIRED)

set(Box2D_STATIC_LIBRARIES TRUE)
set(Box2D_DIR F:/box2d-2.4.1/build/src/CMakeFiles/Export/1f7e0de6fbfb5d20e365871ec89ce73e)

find_package(Box2D COMPONENTS box2d REQUIRED)

set(jsonCpp_STATIC_LIBRARIES TRUE)
set(jsonCpp_DIR F:/jsoncpp/build)

find_package(jsonCpp COMPONENTS jsoncpp REQUIRED)

include_directories(F:/SFML-2.6.0-windows-gcc-13.1.0-mingw-32-bit/SFML-2.6.0/include F:/box2d-2.4.1/include F:/jsoncpp/include include)
target_link_libraries(BlackEngineProject F:/jsoncpp/build/lib/libjsoncpp.a F:/box2d-2.4.1/build/src/lib/libbox2d.a sfml-system sfml-window sfml-graphics sfml-audio)