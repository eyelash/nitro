#include "atmosphere.hpp"
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
	return -x*x + 2.0*x;
}

float atmosphere::OscillatingAnimation::get_y(float x) const {
	return -cos(x*M_PI) * 0.5 + 0.5;
}

float atmosphere::SwayAnimation::get_y(float x) const {
	// -2x^3 + 3x^2
	return -2.0*x*x*x + 3.0*x*x;
}

long atmosphere::Animation::time;
std::vector<atmosphere::Animation> atmosphere::Animation::animations;

atmosphere::Animation::Animation(float from, float to, long duration, const ApplyFunction& apply_function, const AnimationType* type): start_value(from), end_value(to), start_time(time), duration(duration), apply_function(apply_function), type(type) {

}

bool atmosphere::Animation::apply() {
	float x = (float)(time-start_time)/duration;
	if (x >= 1.f) {
		apply_function(end_value);
		return true;
	}
	x = type->get_y(x);
	apply_function(start_value + x * (end_value-start_value));
	return false;
}

void atmosphere::Animation::animate(float from, float to, long duration, const ApplyFunction& apply_function, const AnimationType* type) {
	animations.push_back(Animation(from, to, duration, apply_function, type));
}

void atmosphere::Animation::set_time(long time) {
	Animation::time = time;
}

void atmosphere::Animation::apply_all() {
	auto it = std::remove_if(animations.begin(), animations.end(), [](Animation& animation) {
		return animation.apply();
	});
	animations.erase(it, animations.end());
}
