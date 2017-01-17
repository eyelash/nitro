/*

Copyright (c) 2016-2017, Elias Aebi
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <vector>

namespace atmosphere {

template <class T> struct _Identity {
	using type = T;
};
template <class T> using nondeduced = typename _Identity<T>::type;

template <class T> class Property {
	void* object;
	using Getter = T (*)(void*);
	using Setter = void (*)(void*, T);
	Getter getter;
	void (*setter)(void* object, T value);
public:
	template <class C> Property(C* object, nondeduced<T(*)(C*)> getter, nondeduced<void(*)(C*,T)> setter): object(object), getter(reinterpret_cast<Getter>(getter)), setter(reinterpret_cast<Setter>(setter)) {

	}
	T get() {
		return getter(object);
	}
	void set(T value) {
		setter(object, value);
	}
};

// create properties from getter and setter methods
template <class T, class C, T (C::*Get)() const> T _get(C* object) {
	return (object->*Get)();
}
template <class T, class C, void (C::*Set)(T)> void _set(C* object, T value) {
	(object->*Set)(value);
}
template <class T, class C, T (C::*Get)() const, void (C::*Set)(T)> Property<T> create_property(C* object) {
	return Property<T>(object, &_get<T, C, Get>, &_set<T, C, Set>);
}

template <class T> constexpr T linear(const T& v1, const T& v2, float x) {
	return v1 * (1.f - x) + v2 * x;
}

class AnimationType {
public:
	virtual float get_y (float x) const = 0;
	static const AnimationType* LINEAR;
	static const AnimationType* ACCELERATING;
	static const AnimationType* DECELERATING;
	static const AnimationType* OSCILLATING;
	static const AnimationType* SWAY;
};
class LinearAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};
class AcceleratingAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};
class DeceleratingAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};
class OscillatingAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};
class SwayAnimation: public AnimationType {
public:
	float get_y(float x) const override;
};

class Animation {
	static std::vector<Animation*> animations;
public:
	static long time;
	virtual bool apply() = 0;
	static void add_animation(Animation* animation);
	static void set_time(long time);
	static void apply_all();
};

template <class T> class Animator: public Animation {
	T start_value, end_value;
	long start_time, duration;
	Property<T> property;
	const AnimationType* type;
	bool running;
public:
	Animator(const Property<T>& property): property(property), running(false) {

	}
	void animate(T to, long duration, const AnimationType* type = AnimationType::SWAY) {
		start_value = property.get();
		end_value = to;
		start_time = Animation::time;
		this->duration = duration;
		this->type = type;
		if (!running) {
			running = true;
			Animation::add_animation(this);
		}
	}
	bool apply() {
		float x = (float)(time-start_time)/duration;
		if (x >= 1.f) {
			property.set(end_value);
			running = false;
			return true;
		}
		x = type->get_y(x);
		property.set(linear(start_value, end_value, x));
		return false;
	}
};

}
