#include <atmosphere.hpp>
using namespace atmosphere;

int main() {
	Window window {800, 600, "test"};
	Rectangle rect1 {100, 100, 200, 300, {0, 1, 0, 1}};
	rect1.clipping = true;
	RoundedRectangle rect2 {100, 100, 200, 100, {0, 0, 0, 0.5f}, 16};
	window.add_child(&rect1);
	rect1.add_child(&rect2);
	Animator<float> animator {rect1.position_x()};
	animator.animate(200, 2000);
	window.run();
}
