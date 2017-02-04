#include <atmosphere.hpp>
using namespace atmosphere;

int main() {
	Window window {800, 500, "demo"};

	Rectangle background {0, 0, 800, 500, Color::create(0.9, 0.9, 0.9)};
	background.set_padding(100);
	window.set_child(&background);

	BlurredRectangle blurred_rectangle {Color::create(0, 0, 0, 0.2), 8, 20};
	background.set_child(&blurred_rectangle);

	RoundedRectangle rounded_rectangle {450, 100, 250, 150, Color::create(0.6, 0.9, 0.6), 10};
	blurred_rectangle.set_child(&rounded_rectangle);

	RoundedBorder border {0, 0, 0, 0, 2, Color::create(0, 0, 0, 0.2), 10};
	rounded_rectangle.set_child(&border);

	//Font font {"/usr/share/fonts/truetype/roboto/hinted/Roboto-Regular.ttf", 16};
	//TextContainer text {&font, "some text", Color::create(0, 0, 0, 0.6), 250, 150};
	//border.set_child(&text);

	window.run();
}
