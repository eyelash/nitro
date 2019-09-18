/*

Copyright (c) 2016-2019, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <cstdint>
#include <vector>

namespace nitro {

template <class T> class Property {
	void* object;
	using Getter = T (*)(void*);
	using Setter = void (*)(void*, T);
	Getter getter;
	Setter setter;
public:
	Property(void* object, Getter getter, Setter setter): object(object), getter(getter), setter(setter) {}
	T get() const {
		return getter(object);
	}
	void set(T value) const {
		setter(object, value);
	}
};

// create properties from getter and setter methods
template <class T, class C, T (C::*Get)() const, void (C::*Set)(T)> Property<T> create_property(C* object) {
	return Property<T>(object, [](void* object) -> T {
		return (static_cast<C*>(object)->*Get)();
	}, [](void* object, T value) {
		(static_cast<C*>(object)->*Set)(value);
	});
}
template <class T, class C, const T& (C::*Get)() const, void (C::*Set)(const T&)> Property<T> create_property(C* object) {
	return Property<T>(object, [](void* object) -> T {
		return (static_cast<C*>(object)->*Get)();
	}, [](void* object, T value) {
		(static_cast<C*>(object)->*Set)(value);
	});
}

template <class T> constexpr T linear(const T& v1, const T& v2, float x) {
	return v1 * (1.f - x) + v2 * x;
}

class AnimationType {
public:
	float (*get_y)(float x);
	AnimationType() {}
	constexpr AnimationType(float (*get_y)(float)): get_y(get_y) {}
	static const AnimationType LINEAR;
	static const AnimationType ACCELERATING;
	static const AnimationType DECELERATING;
	static const AnimationType OSCILLATING;
	static const AnimationType SWAY;
};

class Animation {
	static std::vector<Animation*> animations;
	static std::uint64_t time;
public:
	virtual ~Animation() {}
	virtual bool apply() = 0;
	static void add_animation(Animation* animation);
	static std::uint64_t get_time();
	static void advance_time(float seconds);
	static void apply_all();
};

template <class T> class Animator: public Animation {
	T start_value, end_value;
	std::uint64_t start_time, duration;
	Property<T> property;
	AnimationType type;
	bool running;
public:
	Animator(const Property<T>& property): property(property), running(false) {}
	void animate(T to, float duration, AnimationType type = AnimationType::SWAY) {
		start_value = property.get();
		end_value = to;
		start_time = get_time();
		this->duration = duration * 1000000.f + .5f;
		this->type = type;
		if (!running) {
			running = true;
			Animation::add_animation(this);
		}
	}
	bool apply() override {
		const float x = static_cast<float>(get_time() - start_time) / duration;
		if (x >= 1.f) {
			property.set(end_value);
			running = false;
			return true;
		}
		property.set(linear(start_value, end_value, type.get_y(x)));
		return false;
	}
};

}
