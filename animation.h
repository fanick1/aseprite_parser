/*
 * Aseprite animation
 * Version 0.1
 * Copyright 2018 by Frantisek Veverka
 *
 */

#ifndef ANIMATION_H
#define ANIMATION_H

#include <cstdint>

#include <string>
#include <iostream>

#include <array>
#include <vector>
#include "aseprite.h"
namespace animation{

enum class LoopType {
	FORWARD,
	REVERSE,
	PING_PONG
};



struct Color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a = 255;

	Color & operator = (const aseprite::Color & src) {
		r = src.r;
		g = src.g;
		b = src.b;
		a = src.a;
		return *this;
	}
};

class Palette {
public:
	std::array<Color, 256> colors;
};

class Image {
public:
	uint16_t width;
	uint16_t height;

	std::vector<uint8_t> pixels; // TOOD 32bit color images
};

class Cel {
public:
	uint16_t x;
	uint16_t y;

	uint8_t opacity;

	Image * image;
};

class Frame {
public:
	uint16_t duration;//ms
	//Cel cel;
};
class Layer {
public:
	bool visible = true;
	bool isGroupLayer = false; // true -> no frames
	uint8_t opacity;
	std::string name;
	std::vector<Cel> frames;
	Layer(bool visible, bool isGroupLayer, uint8_t opacity, const std::string & name, uint32_t framesCount):
		visible(visible),
		isGroupLayer(isGroupLayer),
		opacity(opacity),
		name(name) {
		if(!isGroupLayer){
			frames.resize(framesCount);
		}
	}
	Layer(const Layer & l) = default;
	Layer(Layer && l) = default;
	Layer & operator = (const Layer & l) = default;
};
class Loop {
public:
	uint16_t from;
	uint16_t to;
	LoopType loopType;
	std::string name;

	Loop(uint16_t from, uint16_t to, LoopType loopType, const std::string & name):
		from(from),
		to(to),
		loopType(loopType),
		name(name) {
	}
};
class Animation {
public:
	uint16_t width;
	uint16_t height;
	uint16_t framesCount;
	Palette palette;
	std::vector<Frame> frames;
	std::vector<Layer> layers;
	std::vector<Image> images;
	std::vector<Loop> loops;
	void log(){
		std::cout << "Animation: frames:" << framesCount << " width: " << width << " height: " << height << "\n";
		std::cout << "  layers: \n";
		for(const auto & layer : layers){
			std::cout << " name: " << layer.name << " " << (layer.isGroupLayer ? "[G]" : "") << "\n";
		}
	}
};


}
#endif
