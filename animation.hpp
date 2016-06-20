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
		property.set(start_value * (1.f-x) + end_value * x);
		return false;
	}
};

}
