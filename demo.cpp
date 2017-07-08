#include <nitro.hpp>
using namespace nitro;

int main() {
	Window window {800, 500, "demo"};

	Rectangle background {Color::create(0.9, 0.9, 0.9)};
	background.set_padding(100);
	window.set_child(&background);

	BlurredRectangle blurred_rectangle {Color::create(0, 0, 0, 0.2), 8, 20};
	background.set_child(&blurred_rectangle);

	RoundedRectangle rounded_rectangle {Color::create(0.2, 0.9, 0.7), 10};
	blurred_rectangle.set_child(&rounded_rectangle);

	RoundedBorder border {2, Color::create(0, 0, 0, 0.2), 10};
	rounded_rectangle.set_child(&border);

	FontSet font {"Roboto", 16};
	TextContainer text {&font, "some text", Color::create(0, 0, 0, 0.5)};
	border.set_child(&text);

	window.run();
}
