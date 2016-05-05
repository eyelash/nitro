#include <atmosphere.hpp>
using namespace atmosphere;

int main() {
	Window window {800, 600, "test"};
	Rectangle rect1 {100, 100, 200, 300, {0, 1, 0, 1}};
	rect1.clipping = true;
	Rectangle rect2 {100, 100, 200, 100, {1, 0, 0, 1}};
	window.add_child(&rect1);
	rect1.add_child(&rect2);
	window.run();
}
