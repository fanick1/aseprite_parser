/*
 * main.cpp
 *
 *  Created on: Sep 18, 2018
 *      Author: fanick
 *
 *      TODO: code formatting
 *
 */
// https://github.com/aseprite/aseprite/blob/af32df1a2aa1b243effc5509f766276164d57c05/docs/ase-file-specs.md
//  aseprite/docs/ase-file-specs.md

// aseprite format header
#ifndef ASEPRITE_H
#define ASEPRITE_H
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <array>
#include <memory>
#include <variant>
#include "tinf/tinf.h"

using BYTE = uint8_t;
using WORD = uint16_t;
using SHORT = int16_t;
using DWORD = uint32_t;
using LONG = int32_t;
using FIXED = int32_t; // convert 16.16 fixed to double: x / 65536
template <typename OUT>
bool operator & (std::ifstream & stream, OUT & out){
    stream.read((char *) &out, sizeof(OUT));
    return stream.good();
}

template <typename OUT, size_t S>
bool operator & (std::ifstream & stream, OUT (& out)[S]){
    stream.read((char *) &out, sizeof(OUT) * S);
        return stream.good();

    return stream.good();
}
enum PIXELTYPE {
    RGBA, GRAYSCALE, INDEXED
};
union PIXEL_DATA {
    BYTE RGBA[4];
    BYTE GRAYSCALE[2];
    BYTE INDEXED;
};

struct PIXEL {
    PIXELTYPE TYPE;
    PIXEL_DATA DATA;
};
struct STRING {
    STRING(){};
    STRING(STRING && s): length(s.length), data(std::move(s.data)){

    }
    STRING & operator = (const STRING && s){
        data = std::move(s.data);
        return *this;
    }
    WORD length;
    std::vector<BYTE> data;
    bool read(std::ifstream & s){
        bool result = s & length;
        if(!result) return result;
        data.resize(length);
        for (size_t i = 0; i < length && result; i++){
            result = result && s & data[i];
        }
        return result;
    }
};

template <>
bool operator & (std::ifstream & s, STRING & string){
    return string.read(s);
}
struct ASE_HEADER {
public:
    DWORD fileSize; // file size
    WORD magicNumber; //    (0xA5E0)
    WORD frames;
    WORD width; //pixels
    WORD height; //pixels
    WORD bitDepth; //8 / 16/ 32
    DWORD flags; // 1 - layer opacity has valid value

    WORD speed; // in milliseconds. deprecated use frame duration
    DWORD stuff_0; // zero
    DWORD stuff_1; // zero
    BYTE transparentIndex; // palette index of transparent color in 8 bit sprites

    BYTE stuff_2[3]; //unused
    WORD colorsCount; // 0 = 256

    BYTE pixelWidth; //if zero aspect is 1:1
    BYTE pixelHeight; //if zero aspect is 1:1
    BYTE reserved[92]; // future

public:

    bool read(std::ifstream & s){ //order must match order of member variables

       return s & fileSize
            && s & magicNumber
            && s & frames
            && s & width
            && s & height
            && s & bitDepth
            && s & flags
            && s & speed
            && s & stuff_0
            && s & stuff_1
            && s & transparentIndex
            && s & stuff_2
            && s & colorsCount
            && s & pixelWidth
            && s & pixelHeight
            && s & reserved;
    }
    void toString(){
        std::cout << "HEADER\n"
               "fileSize           :" << fileSize        << "\n"
               "magicNumber        :" << std::hex << magicNumber   <<std::dec  << "\n"
               "frames             :" << frames          << "\n"
               "width              :" << width           << "\n"
               "height             :" << height          << "\n"
               "bitDepth           :" << bitDepth        << "\n"
               "flags              :" << flags           << "\n"
               "speed              :" << speed           << "\n"
               "stuff_0            :" << stuff_0         << "\n"
               "stuff_1            :" << stuff_1         << "\n"
               "transparentIndex   :" << (int)transparentIndex<< "\n"
               "stuff_2            :(len)" << sizeof(stuff_2)         << "\n"
               "colorsCount        :" << colorsCount     << "\n"
               "pixelWidth         :" << (int)pixelWidth      << "\n"
               "pixelHeight        :" << (int)pixelHeight     << "\n"
               "reserved           :(len)" << sizeof(reserved)        << "\n";
    }
};
template <>
bool operator & (std::ifstream & s, ASE_HEADER & header){
    return header.read(s);
}



enum CHUNK_TYPE {
    PALETTE_OLD_0x0004 = 0x0004,
    PALETTE_OLD_0x0011 = 0x0011,
          LAYER_0x2004 = 0x2004,
            CEL_0x2005 = 0x2005,
      CEL_EXTRA_0x2006 = 0x2006,
           MASK_0x2016 = 0x2016, // 0x2016 DEPRECATED
           PATH_0x2017 = 0x2017, // 0x2017 never used
     FRAME_TAGS_0x2018 = 0x2018,
        PALETTE_0x2019 = 0x2019,
      USER_DATA_0x2020 = 0x2020, //TODO
          SLICE_0x2022 = 0x2022,
};
struct Color {
    BYTE r;
    BYTE g;
    BYTE b;
    BYTE a = 255;
};
struct PALETTE_OLD_CHUNK {
    PALETTE_OLD_CHUNK(){}

    PALETTE_OLD_CHUNK(PALETTE_OLD_CHUNK && p):colors(std::move(p.colors)){

    }

    PALETTE_OLD_CHUNK(std::ifstream & s){
        read(s);
    }

    PALETTE_OLD_CHUNK & operator = (const PALETTE_OLD_CHUNK && palatte) {
        colors = std::move(palatte.colors);
        return *this;
    }
    std::array<Color, UINT8_MAX + 1> colors;

    bool read(std::ifstream & s){
        WORD packets;
        bool result = s & packets;
        WORD lastIndex = 0;

        if (result) {
            for (WORD i = 0; i < packets; i++) {
                BYTE skip;
                BYTE colorsCount = 0;
                result = result && s & skip
                        && s & colorsCount;
                WORD colorNum = colorsCount == 0 ? 256 : colorsCount;
                if (!result) return result;
                lastIndex += skip;
                for (WORD c = 0; c < colorNum; c++) {
                    if (lastIndex > 255) return false;
                    Color & color = colors[lastIndex++];
                    result = result && s & color.r
                            && s & color.g
                            && s & color.b;

                }
            }
        }
        return result;
    }
};
struct PALETTE_CHUNK {
    std::array<Color, UINT8_MAX + 1> colors;

    PALETTE_CHUNK(PALETTE_CHUNK && palette):colors(std::move(palette.colors)){

    }
    PALETTE_CHUNK(std::ifstream & s){
        read(s);
    }

    PALETTE_CHUNK & operator = (const PALETTE_CHUNK && palette) {
    	colors = std::move(palette.colors);
    	return *this;
    }
    ~PALETTE_CHUNK(){}
    bool read (std::ifstream & s){
        DWORD newSize; // total number of entries
        DWORD first;
        DWORD last;
        BYTE unused[8];

        bool result = s & newSize
                && s & first
                && s & last
                && s & unused;
        if(!result) return result;
        for (DWORD i = first; i <= last && result; i++) {
            if(i > 255) return false; // we don't support such fancy graphics
            Color & c = colors[i];
            WORD flags;
            STRING foobar;
            result = result && s & flags
                    && s & c.r
                    && s & c.g
                    && s & c.b
                    && s & c.a;
            if(result && flags & 0x1){
                result = result && s & foobar; // throw away the color name
            }
        }
        return result;
    }
};

struct LAYER_CHUNK {

	LAYER_CHUNK(LAYER_CHUNK && layer) noexcept{
		//TODO
	}
    LAYER_CHUNK(std::ifstream & s){
        read(s);
    }
    LAYER_CHUNK & operator = (const LAYER_CHUNK && layer) {
        flags = layer.flags;
        layerType = layer.layerType;
        layerChildLevel = layer.layerChildLevel;
        width = layer.width;
        height = layer.height;
        blendMode = layer.blendMode;
        opacity = layer.opacity;
        name = std::move(layer.name);
        return *this;
    }
    WORD flags;
    WORD layerType; //0 - normal, 1 - group layer
    WORD layerChildLevel;

    WORD width; // default layer width in pixels . ignored
    WORD height; //ignored

    WORD blendMode; //
                    //                  Normal         = 0
                    //                  Multiply       = 1
                    //                  Screen         = 2
                    //                  Overlay        = 3
                    //                  Darken         = 4
                    //                  Lighten        = 5
                    //                  Color Dodge    = 6
                    //                  Color Burn     = 7
                    //                  Hard Light     = 8
                    //                  Soft Light     = 9
                    //                  Difference     = 10
                    //                  Exclusion      = 11
                    //                  Hue            = 12
                    //                  Saturation     = 13
                    //                  Color          = 14
                    //                  Luminosity     = 15
                    //                  Addition       = 16
                    //                  Subtract       = 17
                    //                  Divide         = 18
    BYTE opacity;
    BYTE unused[3];
    STRING name;

    bool read(std::ifstream & s){
        return s & flags
                && s & layerType
                && s & layerChildLevel
                && s & width
                && s & height
                && s & blendMode
                && s & opacity
                && s & unused
                && s & name;
    }

};
struct TAG {
    TAG(){}
    TAG(TAG && t):from(t.from),
            to(t.to),
            direction(t.direction),
            name(std::move(t.name)){

    }
    TAG & operator = (const TAG && t){
    	from = t.from;
    	to = t.to;
    	direction = t.direction;
    	name = std::move(t.name);
    	return *this;
    }
    WORD from;
    WORD to;
    BYTE direction; // 0 - forward, 1 - reverse, 2 - ping-pong
//    BYTE unused[8];
//    BYTE color[3]; // unused, color of the tag
//    BYTE extra; //ignored, 0
    STRING name; //name of the animatiom loop (e.g. walk, jump, etc.)
    bool read(std::ifstream & s) {
        BYTE unused[8];
        BYTE color[3]; // unused, color of the tag
        BYTE extra; //ignored, 0
        return s & from
                && s & to
                && s & direction
                && s & unused
                && s & color
                && s & extra
                && s & name;
    }
};
struct TAG_CHUNK {
    std::vector<TAG> tags;
    TAG_CHUNK(std::ifstream & s){
        read(s);
    }

    TAG_CHUNK(TAG_CHUNK && tag):tags(std::move(tag.tags)){}
    TAG_CHUNK & operator = (TAG_CHUNK && tag){
    	tags = std::move(tag.tags);
    	return *this;
    }
    bool read (std::ifstream & s) {
        WORD count;
        BYTE future[8]; // unused
        bool result = s & count
                && s & future;
        tags.reserve(count);
        for (WORD i = 0; i < count && result; ++ i){
            TAG t;
            result = result && t.read(s);
            if(result) tags.push_back(std::move(t));
        }

        return result;
    }
};
struct SLICE_KEY {
    struct TYPE_NINE_PATCHES { // 9-patches slice - a 3x3 grid
        LONG centerX;
        LONG centerY;
        DWORD centerWidth;
        DWORD centerHeight;
    };
    struct TYPE_PIVOT {
        LONG pivotX;
        LONG pivotY;
    };
    DWORD frame;
    LONG x;
    LONG y;
    DWORD width;
    DWORD height;
    union t_data {
        TYPE_NINE_PATCHES ninePatches;
        TYPE_PIVOT pivot;
    };
    t_data data;
};
struct SLICE_CHUNK {
    std::vector<SLICE_KEY> sliceKeys;
    DWORD count;
    DWORD flags;
    STRING name;
    SLICE_CHUNK(std::ifstream & s){
        read(s);
    }
    bool read(std::ifstream & s){
        DWORD reserved;
        bool result = s & count
                && s & flags
                && s & reserved
                && s & name;
        if(!result) return result;
        sliceKeys.resize(count);
        for(DWORD i = 0; i < count && result; i++){
            auto & k = sliceKeys[i];
            result = result && s & k.frame
            && s & k.x
            && s & k.y
            && s & k.width
            && s & k.height;
            switch(flags){
            case 1:{
                auto & n = k.data.ninePatches;
                result = result && s & n.centerX
                        && s & n.centerY
                        && s & n.centerWidth
                        && s & n.centerHeight;
                break;
            }
            case 2:{
                auto & pivot = k.data.pivot;
                result = result && s & pivot.pivotX
                        && s & pivot.pivotY;
                break;}
            case 0:{
                break;
            }
            }
        }
        return result;
    }
};

struct CEL_CHUNK {

	//TODO BIG TIME!!!
	CEL_CHUNK & operator = (const CEL_CHUNK && cel){
		return *this;
	}
	//TODO BIG TIME!!!
	CEL_CHUNK(CEL_CHUNK && c){}
    CEL_CHUNK(std::ifstream & s, PIXELTYPE pixelFormat, DWORD dataSize){
        read(s, pixelFormat, dataSize);
    }
    ~CEL_CHUNK(){
        pixels.clear();
    }
    WORD layerIndex;
    SHORT x;
    SHORT y;
    BYTE opacity;
    WORD type;
    std::vector<PIXEL_DATA> pixels;

    WORD width = 0; //type == 0,2
    WORD height = 0; // type == 0,2
    WORD frameLink; // type == 1
    // chunkSize - to tell size of compressed data
    bool read (std::ifstream & s, PIXELTYPE pixelFormat, DWORD dataSize){
        BYTE reserved[7];
        bool result = s & layerIndex
                && s & x
                && s & y
                && s & opacity
                && s & type
                && s & reserved;
        if(!result) return result;
        switch (type){
        case 0:{
            result = readRawPixels(s, pixelFormat);
            break;
        }
        case 1:{
            result = s & frameLink;
            break;
        }
        case 2: {
            result = readCompressedPixels(s, pixelFormat, dataSize - 16 /*size of already read data*/);
            break;
        }
        default: result = false;
        }

        return result;
    }

    bool readRawPixels(std::ifstream & s, PIXELTYPE pixelFormat){
        bool result = s & width && s & height;
        if(!result) return result;
        DWORD dim = width * height;
        pixels.resize(dim);

        switch(pixelFormat){
            case INDEXED:{
                for(DWORD i = 0; (i < dim) && result; i++) {
                    result = result && s & pixels[i].INDEXED;
                }
                break;
            }
            case GRAYSCALE:{
                for(DWORD i = 0; (i < dim) && result; i++) {
                    result = result && s & pixels[i].GRAYSCALE;
                }
                break;
            }
            case RGBA:{
                for(DWORD i = 0; i < dim && result; i++) {
                    result = result && s & pixels[i].RGBA;
                }
                break;
            }
            default:
                result = false;
        }

        return result;
    }
    bool readCompressedPixels(std::ifstream & s, PIXELTYPE pixelFormat, DWORD sourceLen){
        bool result = s & width && s & height;
        if(!result) return result;
        DWORD dim = width * height;
        pixels.resize(dim);
        sourceLen -= 4; /* width, height */
        std::vector<BYTE> source(sourceLen);
        s.read((char*)source.data() /*zlib header*/, sourceLen);

        result = s.good();
        if(!result){
            return result;
        }


        DWORD expectedLen = dim;
        switch (pixelFormat) {
            case INDEXED: {
                break;
            }
            case GRAYSCALE: {
                expectedLen *= 2;
                break;
            }
            case RGBA: {
                expectedLen *= 4;
                break;
            }
            default:
                result = false;
        }
        std::vector<BYTE> uncompressed(expectedLen);
        DWORD destLen;
        auto outcome = tinf_uncompress(uncompressed.data(), &destLen, source.data() + 2, sourceLen -2 - 4/*zlib header, crc*/);
        result = result && TINF_OK == outcome;

        result = result && destLen == expectedLen;

        if(!result) return result;
        switch (pixelFormat) {
                   case INDEXED: {
                       for(DWORD i = 0 ; i < dim; i++) {
                           pixels[i].INDEXED = uncompressed[i];
                       }
                       break;
                   }
                   case GRAYSCALE: {
                       for(DWORD i = 0 ; i < dim; i++) {
                           pixels[i].GRAYSCALE[0] = uncompressed[2 * i];
                           pixels[i].GRAYSCALE[1] = uncompressed[2 * i + 1];
                       }
                       break;
                   }
                   case RGBA: {
                       for(DWORD i = 0 ; i < dim; i++) {
                           pixels[i].RGBA[0] = uncompressed[4 * i];
                           pixels[i].RGBA[1] = uncompressed[4 * i + 1];
                           pixels[i].RGBA[2] = uncompressed[4 * i + 2];
                           pixels[i].RGBA[3] = uncompressed[4 * i + 3];
                       }
                       break;
                   }
        }
        return result;
    }
};

struct CHUNK {

	// all variants must have move constructor, move assignment operator
	// first variant must have default constructor
	using chunk_t = std::variant<
			PALETTE_OLD_CHUNK,
		    LAYER_CHUNK ,
			PALETTE_CHUNK,
		    CEL_CHUNK,
		    TAG_CHUNK,
		    SLICE_CHUNK>;
    chunk_t data;
    WORD type;

    CHUNK(chunk_t && data, WORD type):data(std::move(data)), type(type){

    }

    CHUNK(CHUNK && c) {
        std::cout << "CHUNK(CHUNK && ) called :( \n";
        data = std::move(c.data);
        type = c.type;
        c.type = 0;
    }

    ~CHUNK() {
        type = 0;
    }
};

struct FRAME {
    DWORD size; // bytes
    WORD magicNumber; //0xF1FA
    WORD chunks_old; // if 0xFFFF use chunks field (the new one below};
    WORD duration; // in milliseconds
    BYTE reserved[2]; // 0
    DWORD chunkCount; // if zero, use chunks_old
    std::vector<CHUNK> chunks;
    bool read(std::ifstream & s, PIXELTYPE pixelFormat){
        bool result = s & size
                && s & magicNumber
                && s & chunks_old
                && s & duration
                && s & reserved
                && s & chunkCount;

        if(result){
            std::cout <<std::hex<< "DEBUG Frame: magicNumber: " << magicNumber << " chunks_old: " << chunks_old << std::dec << "\n";
            //chunks.reserve(chunkCount);
            for(size_t c = 0; c < chunkCount && result; c++){
                DWORD size;
                WORD type;
                auto p = s.tellg();
                result = result && s & size && s & type;
                auto p2 = s.tellg();
                std::cout <<std::hex<< "0x" << p2 << ":DEBUG Chunk: size: " << size << " type: " << type << std::dec << "\n";
                switch (type){
                    case PALETTE_OLD_0x0004:{
                    case PALETTE_OLD_0x0011:
                        chunks.emplace_back(PALETTE_OLD_CHUNK(s), type);
                        break;}
                    case LAYER_0x2004:{;
                    	chunks.emplace_back(LAYER_CHUNK(s), type);
                        break;
                    }
                    case CEL_0x2005:{
                        chunks.emplace_back(CEL_CHUNK(s, pixelFormat, size - 6), type);
                        break;}
                    case FRAME_TAGS_0x2018:{
                    	chunks.emplace_back(TAG_CHUNK(s), type);
                        break;}
                    case PALETTE_0x2019:{
                        chunks.emplace_back(PALETTE_CHUNK(s), type);
                        break;}
                    case SLICE_0x2022:{
                        chunks.emplace_back(SLICE_CHUNK(s), type);
                        break;}
                    default:
                        std::cout << "^ not parsed\n";
                        s.seekg(size + p); // skip data
                }
                result = result && s.good();
            }
        }
        return result;
    }
};


struct ASEPRITE {
    ASE_HEADER header;
    std::vector<FRAME> frames;
    std::ifstream file;
    ASEPRITE(std::string filename) :
            file(filename, std::ios::in | std::ios::binary) {
        if (!file.good()) {
            std::cout << "File " << filename << " not good\n";
            file.close();
            return;
        }

        if (readAseHeader()) {
            header.toString();
            PIXELTYPE pixelFormat = header.bitDepth == 8 ? INDEXED : header.bitDepth == 16 ? GRAYSCALE : RGBA;
            frames.resize(header.frames);
            for(size_t f = 0; f < header.frames && file.good() ; f++){
                std::cout << " FRAME " << f << "\n";
                frames[f].read(file, pixelFormat);
            }
        }

        file.close();

    }
private:
    bool readAseHeader() {
        return file & header;
    }
};


/*
 Notes
NOTE.1

The child level is used to show the relationship of this layer with the last one read, for example:

Layer name and hierarchy      Child Level
-----------------------------------------------
- Background                  0
  `- Layer1                   1
- Foreground                  0
  |- My set1                  1
  |  `- Layer2                2
  `- Layer3                   1

NOTE.2

The layer index is a number to identify any layer in the sprite, for example:

Layer name and hierarchy      Layer index
-----------------------------------------------
- Background                  0
  `- Layer1                   1
- Foreground                  2
  |- My set1                  3
  |  `- Layer2                4
  `- Layer3                   5

 */
#endif
