#include <atmosphere.hpp>
using namespace atmosphere;

int main() {
	Window window {800, 600, "demo"};
	Rectangle background {0, 0, 800, 600, {0.9, 0.9, 0.9}};
	window.set_child(&background);

	Rectangle rectangle {100, 100, 250, 150, {0.3, 0.3, 0.5}};
	background.add_child(&rectangle);
	TextContainer rectangle_text {"Rectangle", {1, 1, 1, 0.9}, 250, 150};
	rectangle.add_child(&rectangle_text);

	RoundedRectangle rounded_rectangle {450, 100, 250, 150, {0.3, 0.3, 0.5}, 10};
	background.add_child(&rounded_rectangle);
	TextContainer rounded_rectangle_text {"RoundedRectangle", {1, 1, 1, 0.9}, 250, 150};
	rounded_rectangle.add_child(&rounded_rectangle_text);

	window.run();
}
