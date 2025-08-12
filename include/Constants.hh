const unsigned int WINDOW_WIDTH{760};
const unsigned int WINDOW_HEIGHT{760};
const char* GAME_NAME{"Game1"};
const char* ASSETS_SPRITES{"assets/sprites.png"};
const char* ASSETS_TILES{"assets/tiles.png"};
const char* ASSETS_MAPS{"assets/maps/level1.grid"};
const char* ASSETS_MAPS_NEW{"assets/maps/map_20250809_183540.grid"};
const char* ASSETS_MAPS_JSON{"assets/maps/level1.json"};
const char* ASSETS_MAPS_JSON_TWO{"assets/maps/level2.json"};
const char* ASSETS_MAPS_JSON_THREE{"assets/maps/level3.json"};
const char* ASSETS_FONT_ARCADECLASSIC{"assets/fonts/ARCADECLASSIC.TTF"};

// Game constants
namespace GameConstants {
    constexpr float PLAYER_SPEED = 200.0f;
    constexpr float PLAYER_FRICTION = 0.28f;
    constexpr int PHYSICS_VELOCITY_ITERATIONS = 8;
    constexpr int PHYSICS_POSITION_ITERATIONS = 8;
    constexpr float TILE_SIZE = 16.0f;
    constexpr float TILE_SCALE = 4.0f;
    constexpr int MAP_WIDTH = 12;
    constexpr int MAP_HEIGHT = 12;
}