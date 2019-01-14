/*
 * main.cpp
 *  - aseprite parser example
 *  Created on: Sep 18, 2018
 *      Author: Frantisek Veverka fanickbux@gmail.com
 */

#include "aseprite_to_animation.h"

int main(){
    tinf_init(); //initialize the tinf library used for decompression
    animation::Animation a = animation::Animation::loadAseImage("mountain.aseprite");
    a.log();
    return 0;
}


