#pragma once
#include<SFML/Graphics.hpp>

#include<iostream>
#include<string>
#include<cstdint>
#include<memory>

class TextObject
{
private:
  sf::Font font{};
  std::unique_ptr<sf::Text> text{};
  std::string fontUrl;
  int size{};
  sf::Color color;
  std::string textStr{};
public:
  TextObject(std::string fontUrl, int size, sf::Color color, std::uint32_t style);
  TextObject(std::string fontUrl, int size, sf::Color color, std::uint32_t style, std::string textStr);
  ~TextObject();
  void SetTextStr(std::string textStr);
  sf::Text* GetText() const;
};