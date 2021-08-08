/*
 * Aseprite to animation transformation
 * Version 0.1
 * Copyright 2018, 2021 by Frantisek Veverka
 *
 */

#ifndef ASEPRITE_TO_ANIMATION_H_
#define ASEPRITE_TO_ANIMATION_H_
#include "aseprite.h"
#include "animation.h"

animation::LoopType from(uint16_t type);

std::vector<uint8_t> from(const std::vector<aseprite::PIXEL_DATA> & in);

animation::Animation fromASEPRITE(const aseprite::ASEPRITE & ase);
#endif /* ASEPRITE_TO_ANIMATION_H_ */
