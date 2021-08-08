/*
 * Aseprite animation
 * Version 0.1
 * Copyright 2019, 2021 by Frantisek Veverka
 *
 */
#include <string>
#include "animation.h"
#include "aseprite.h"
#include "aseprite_to_animation.h"

animation::Animation animation::Animation::loadAseImage(const std::string &path) {
    return fromASEPRITE(aseprite::ASEPRITE(path));
}
animation::LoopType from(uint16_t type) {
    switch (type) {
    case 0:
        return animation::LoopType::FORWARD;
    case 1:
        return animation::LoopType::REVERSE;
    case 2:
        return animation::LoopType::PING_PONG;
    default:
        return animation::LoopType::FORWARD;
    }
}
std::vector<uint8_t> from(const std::vector<aseprite::PIXEL_DATA> & in) {
    std::vector<uint8_t> result(in.size());
    for (size_t i = 0; i < in.size(); i++) {
        result[i] = in[i].INDEXED;
    }
    return result;
}
animation::Animation fromASEPRITE(const aseprite::ASEPRITE & ase) {
    animation::Animation animation;
    animation.width = ase.header.width;
    animation.height = ase.header.height;
    animation.framesCount = ase.header.frames;
    animation.transparentIndex = ase.header.transparentIndex;
    animation.frames.reserve(animation.framesCount);
    animation.slices.reserve(ase.sliceCount);
    for (const auto & chunk : ase.frames[0].chunks) {
        if (chunk.type == aseprite::CHUNK_TYPE::PALETTE_0x2019) {
            const auto & palette_chunk = std::get<aseprite::PALETTE_CHUNK>(chunk.data);
            for (size_t i = 0; i < palette_chunk.colors.size(); i++) {
                animation.palette.colors[i].r = palette_chunk.colors[i].r;
                animation.palette.colors[i].g = palette_chunk.colors[i].g;
                animation.palette.colors[i].b = palette_chunk.colors[i].b;
                animation.palette.colors[i].a = palette_chunk.colors[i].a;
            }
        }
        if (chunk.type == aseprite::CHUNK_TYPE::FRAME_TAGS_0x2018) {
            const auto & tag_chunk = std::get<aseprite::TAG_CHUNK>(chunk.data);
            animation.loops.reserve(tag_chunk.tags.size());
            for (const auto & tag : tag_chunk.tags) {
                animation.loops.emplace_back(
                    tag.from,
                    tag.to,
                    from(tag.direction),
                    tag.name.toString()
                    );
            }
        }
        if (chunk.type == aseprite::CHUNK_TYPE::LAYER_0x2004) {
            const auto & layer = std::get<aseprite::LAYER_CHUNK>(chunk.data);
            animation.layers.emplace_back(
                animation::Layer::BLEND_MODE(layer.blendMode),
                layer.flags & 0x1,
                layer.layerType == 1,
                layer.opacity,
                layer.name.toString(),
                animation.framesCount);
        }
    }
    for (uint32_t f = 0; f < animation.framesCount; f++) {
        animation.frames[f].duration = ase.frames[f].duration;
        for (const auto & chunk : ase.frames[f].chunks) {
            if (chunk.type == aseprite::CHUNK_TYPE::SLICE_0x2022) {
                const auto & slice_chunk = std::get<aseprite::SLICE_CHUNK>(chunk.data);
                std::vector<animation::Slice::Key> sliceKeys;
                sliceKeys.reserve(slice_chunk.sliceKeys.size());
                for (const auto &key : slice_chunk.sliceKeys) {
                    const bool has9Patch = slice_chunk.flags & 0x01;
                    const bool hasPivot = slice_chunk.flags & 0x02;
                    animation::Slice::NinePatch ninePatch;
                    animation::Slice::Pivot pivot;

                    if(has9Patch){
                        ninePatch.x = key.ninePatches.centerX;
                        ninePatch.y = key.ninePatches.centerY;
                        ninePatch.width = key.ninePatches.centerWidth;
                        ninePatch.height = key.ninePatches.centerHeight;
                    }
                    if(hasPivot){
                        pivot.x = key.pivot.pivotX;
                        pivot.y = key.pivot.pivotY;
                    }
                    sliceKeys.emplace_back(animation::Slice::Key{
                        .frame = f,
                        .x = key.x,
                        .y = key.y,
                        .width = key.width,
                        .height = key.height,
                        .ninePatch = ninePatch,
                        .pivot = pivot
                    });
                }

                auto &slice = animation.slices.emplace_back(
                        animation::Slice{
                                .name = slice_chunk.name.toString(),
                                .sliceKeys = std::move(sliceKeys)
                        }
                );
                animation.sliceLookup[slice.name] = animation.slices.size() - 1;
            }

            if (chunk.type == aseprite::CHUNK_TYPE::CEL_0x2005) {
                const auto & cel_chunk = std::get<aseprite::CEL_CHUNK>(chunk.data);
                auto & layer = animation.layers[cel_chunk.layerIndex]; //TODO bounds check
                auto & cel = layer.frames[f];
                if (cel_chunk.type == 1) { // linked cel
                    auto & linkedCel = animation.layers[cel_chunk.layerIndex].frames[cel_chunk.frameLink];
                    cel.image = linkedCel.image;
                    cel.opacity = linkedCel.opacity;
                    cel.x = linkedCel.x;
                    cel.y = linkedCel.y;

                } else {
                    cel.x = cel_chunk.x;
                    cel.y = cel_chunk.y;
                    cel.opacity = cel_chunk.opacity;
                    animation.images.emplace_back(
                        cel_chunk.width,
                        cel_chunk.height,
                        from(cel_chunk.pixels));
                    cel.image = animation.images.size() - 1;
                }
            }
        }
    }
    if(animation.loops.empty() && animation.framesCount > 0){
        std::vector<int32_t> animationLoop;
        uint16_t loopLength = animation.framesCount;
        animationLoop.reserve(loopLength * 2 + 2);
        for (uint16_t frame = 0; frame < loopLength; frame++) {
            animationLoop.push_back(frame);
            animationLoop.push_back(animation.frames[frame].duration);
        }
        animationLoop.push_back(-1);
        animationLoop.push_back(0);
        animation.animations.push_back(animationLoop);
        animation.animationLookup[""] = animation.animations.size() - 1;
    }
    for (const auto & loop : animation.loops) {
        std::vector<int32_t> animationLoop;

        if (loop.loopType == animation::LoopType::FORWARD) {
            uint16_t loopLength = loop.to - loop.from + 1;
            animationLoop.reserve(loopLength * 2 + 2); // format is frame,duration,...,frame,duraion,-1,0

            for (uint16_t frame = loop.from; frame <= /*(!)*/loop.to; frame++) {
                animationLoop.push_back(frame);
                animationLoop.push_back(animation.frames[frame].duration);
            }

        } else if (loop.loopType == animation::LoopType::PING_PONG) {
            uint16_t loopLength = loop.to - loop.from + 1;
            animationLoop.reserve(loopLength * 4); // format is frame,duration,...,frame,duraion,-1,0

            for (uint16_t frame = loop.from; frame < /*(!)*/loop.to; frame++) {
                animationLoop.push_back(frame);
                animationLoop.push_back(animation.frames[frame].duration);
            }
            for (uint16_t frame = loop.to; frame >= loop.from; frame--) {
                animationLoop.push_back(frame);
                animationLoop.push_back(animation.frames[frame].duration);
            }

        } else if (loop.loopType == animation::LoopType::REVERSE) {
            uint16_t loopLength = loop.to - loop.from + 1;
            animationLoop.reserve(loopLength * 2 + 2); // format is frame,duration,...,frame,duraion,-1,0

            for (uint16_t frame = loop.to; frame >= loop.from; frame--) {
                animationLoop.push_back(frame);
                animationLoop.push_back(animation.frames[frame].duration);
            }
        }

        animationLoop.push_back(-1);
        animationLoop.push_back(0);
        animation.animations.push_back(animationLoop);
        animation.animationLookup[loop.name] = animation.animations.size() - 1;
    }
    return animation;
}
