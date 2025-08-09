#include "GUI/TextObject.hh"
#include <iostream>

TextObject::TextObject(std::string fontUrl, int size, sf::Color color, std::uint32_t style)
{
  this->fontUrl = std::move(fontUrl);
  this->size = size;
  this->color = color;

  if (!font.openFromFile(this->fontUrl)) {
    std::cerr << "Failed to load font: " << this->fontUrl << std::endl;
  }
  text = std::make_unique<sf::Text>(font);
  text->setCharacterSize(size);
  text->setFillColor(color);
  text->setStyle(style);
}

TextObject::TextObject(std::string fontUrl, int size, sf::Color color, std::uint32_t style, std::string textStr)
{
  this->fontUrl = std::move(fontUrl);
  this->size = size;
  this->color = color;
  this->textStr = std::move(textStr);

  if (!font.openFromFile(this->fontUrl)) {
    std::cerr << "Failed to load font: " << this->fontUrl << std::endl;
  }
  text = std::make_unique<sf::Text>(font);
  text->setCharacterSize(size);
  text->setFillColor(color);
  text->setStyle(style);
  text->setString(this->textStr);
}

TextObject::~TextObject()
{
}

void TextObject::SetTextStr(std::string textStr)
{
  this->textStr = std::move(textStr);
  if (text) text->setString(this->textStr);
}

sf::Text* TextObject::GetText() const
{
  return text.get();
}