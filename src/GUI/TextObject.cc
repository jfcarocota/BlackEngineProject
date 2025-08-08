#include "GUI/TextObject.hh"
#include <iostream>

TextObject::TextObject(std::string fontUrl, int size, sf::Color color, sf::Uint32 style)
{
  this->fontUrl = fontUrl;
  this->size = size;
  this->color = color;

  if (!font->loadFromFile(fontUrl)) {
    std::cerr << "Failed to load font: " << fontUrl << std::endl;
  }
  text = new sf::Text();
  text->setFont(*font);
  text->setCharacterSize(size);
  text->setFillColor(color);
  text->setStyle(style);
}

TextObject::TextObject(std::string fontUrl, int size, sf::Color color, sf::Uint32 style, std::string textStr)
{
  this->fontUrl = fontUrl;
  this->size = size;
  this->color = color;
  this->textStr = textStr;

  if (!font->loadFromFile(fontUrl)) {
    std::cerr << "Failed to load font: " << fontUrl << std::endl;
  }
  text = new sf::Text();
  text->setFont(*font);
  text->setCharacterSize(size);
  text->setFillColor(color);
  text->setStyle(style);
  text->setString(textStr);
}

TextObject::~TextObject()
{
  if (text) {
    delete text;
  }
  if (font) {
    delete font;
  }
}

void TextObject::SetTextStr(std::string textStr)
{
  this->textStr = textStr;
  text->setString(textStr);
}

sf::Text* TextObject::GetText() const
{
  return text;
}