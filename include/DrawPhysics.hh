#pragma once
#include <box2d/box2d.h>

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <gsl/assert>
#include <gsl/narrow>

class DrawPhysics : public b2Draw {
 private:
  sf::RenderWindow* window{};

 public:
  DrawPhysics(sf::RenderWindow* window);
  ~DrawPhysics();

  /// Convert Box2D's OpenGL style color definition[0-1] to SFML's color
  /// definition[0-255], with optional alpha byte[Default - opaque]
  static sf::Color GLColorToSFML(const b2Color& color,
                                 std::uint8_t alpha = 255) {
    Expects(color.r >= 0.f && color.r <= 1.f);
    Expects(color.g >= 0.f && color.g <= 1.f);
    Expects(color.b >= 0.f && color.b <= 1.f);
    return sf::Color(gsl::narrow_cast<std::uint8_t>(color.r * 255.f),
                     gsl::narrow_cast<std::uint8_t>(color.g * 255.f),
                     gsl::narrow_cast<std::uint8_t>(color.b * 255.f), alpha);
  }

  /// Convert Box2D's vector to SFML vector [Default - scales the vector up by
  /// SCALE constants amount]
  static sf::Vector2f B2VecToSFVec(const b2Vec2& vector,
                                   bool scaleToPixels = true) {
    return sf::Vector2f(vector.x, vector.y);
  }

  /// Draw a closed polygon provided in CCW order.
  void DrawPolygon(const b2Vec2* vertices, int32 vertexCount,
                   const b2Color& color);

  /// Draw a solid closed polygon provided in CCW order.
  void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount,
                        const b2Color& color);

  /// Draw a circle.
  void DrawCircle(const b2Vec2& center, float radius, const b2Color& color);

  /// Draw a solid circle.
  void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis,
                       const b2Color& color);

  /// Draw a line segment.
  void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);

  /// Draw a transform. Choose your own length scale.
  void DrawTransform(const b2Transform& xf);

  // Draw a point
  void DrawPoint(const b2Vec2& p, float size, const b2Color& color);
};