#include "GUI/TextObject.hh"

#include <iostream>

TextObject::TextObject(std::string fontUrl, int size, sf::Color color,
                       std::uint32_t style) {
  this->fontUrl = std::move(fontUrl);
  this->size = size;
  this->color = color;
  Expects(this->size >= 0);

  if (!font.openFromFile(this->fontUrl)) {
    std::cerr << "Failed to load font: " << this->fontUrl << std::endl;
  }
  text = std::make_unique<sf::Text>(font);
  text->setCharacterSize(size);
  text->setFillColor(color);
  text->setStyle(style);
  Ensures(static_cast<bool>(text));
}

TextObject::TextObject(std::string fontUrl, int size, sf::Color color,
                       std::uint32_t style, std::string textStr) {
  this->fontUrl = std::move(fontUrl);
  this->size = size;
  this->color = color;
  this->textStr = std::move(textStr);
  Expects(this->size >= 0);

  if (!font.openFromFile(this->fontUrl)) {
    std::cerr << "Failed to load font: " << this->fontUrl << std::endl;
  }
  text = std::make_unique<sf::Text>(font);
  text->setCharacterSize(size);
  text->setFillColor(color);
  text->setStyle(style);
  text->setString(this->textStr);
  Ensures(static_cast<bool>(text));
}

TextObject::~TextObject() {}

void TextObject::SetTextStr(std::string textStr) {
  this->textStr = std::move(textStr);
  if (text) text->setString(this->textStr);
}

sf::Text* TextObject::GetText() const {
  Expects(static_cast<bool>(text));
  return text.get();
}