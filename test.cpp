#include <atmosphere.hpp>
using namespace atmosphere;

int main() {
	Window window {800, 600, "test"};
	Rectangle rect {0, 0, 300, 200};
	window.add(&rect);
	window.run();
}
