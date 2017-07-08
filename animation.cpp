/*

Copyright (c) 2016, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "animation.hpp"
#include <cmath>
#include <algorithm>

static float linear_get_y(float x) {
	return x;
}

static float accelerating_get_y(float x) {
	return x*x;
}

static float decelerating_get_y(float x) {
	return -x*x + 2.f*x;
}

static float oscillating_get_y(float x) {
	return -cos(x*M_PI) * .5f + .5f;
}

static float sway_get_y(float x) {
	// -2x^3 + 3x^2
	return -2.f*x*x*x + 3.f*x*x;
}

const nitro::AnimationType nitro::AnimationType::LINEAR = AnimationType(&linear_get_y);
const nitro::AnimationType nitro::AnimationType::ACCELERATING = AnimationType(&accelerating_get_y);
const nitro::AnimationType nitro::AnimationType::DECELERATING = AnimationType(&decelerating_get_y);
const nitro::AnimationType nitro::AnimationType::OSCILLATING = AnimationType(&oscillating_get_y);
const nitro::AnimationType nitro::AnimationType::SWAY = AnimationType(&sway_get_y);

long nitro::Animation::time;
std::vector<nitro::Animation*> nitro::Animation::animations;

void nitro::Animation::add_animation(nitro::Animation* animation) {
	animations.push_back(animation);
}

void nitro::Animation::set_time(long time) {
	Animation::time = time;
}

void nitro::Animation::apply_all() {
	auto it = std::remove_if(animations.begin(), animations.end(), [](Animation* animation) {
		return animation->apply();
	});
	animations.erase(it, animations.end());
}
