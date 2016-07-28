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
#include <algorithm>

static atmosphere::LinearAnimation linear_animation;
static atmosphere::AcceleratingAnimation accelerating_animation;
static atmosphere::DeceleratingAnimation decelerating_animation;
static atmosphere::OscillatingAnimation oscillating_animation;
static atmosphere::SwayAnimation sway_animation;
const atmosphere::AnimationType* atmosphere::AnimationType::LINEAR = &linear_animation;
const atmosphere::AnimationType* atmosphere::AnimationType::ACCELERATING = &accelerating_animation;
const atmosphere::AnimationType* atmosphere::AnimationType::DECELERATING = &decelerating_animation;
const atmosphere::AnimationType* atmosphere::AnimationType::OSCILLATING = &oscillating_animation;
const atmosphere::AnimationType* atmosphere::AnimationType::SWAY = &sway_animation;

float atmosphere::LinearAnimation::get_y(float x) const {
	return x;
}

float atmosphere::AcceleratingAnimation::get_y(float x) const {
	return x*x;
}

float atmosphere::DeceleratingAnimation::get_y(float x) const {
	return -x*x + 2.f*x;
}

float atmosphere::OscillatingAnimation::get_y(float x) const {
	return -cos(x*M_PI) * .5f + .5f;
}

float atmosphere::SwayAnimation::get_y(float x) const {
	// -2x^3 + 3x^2
	return -2.f*x*x*x + 3.f*x*x;
}

long atmosphere::Animation::time;
std::vector<atmosphere::Animation*> atmosphere::Animation::animations;

void atmosphere::Animation::add_animation(atmosphere::Animation* animation) {
	animations.push_back(animation);
}

void atmosphere::Animation::set_time(long time) {
	Animation::time = time;
}

void atmosphere::Animation::apply_all() {
	auto it = std::remove_if(animations.begin(), animations.end(), [](Animation* animation) {
		return animation->apply();
	});
	animations.erase(it, animations.end());
}
