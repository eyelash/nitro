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
