#include <atmosphere.hpp>
using namespace atmosphere;

int main() {
	Window window {800, 600, "demo"};
	Rectangle background {0, 0, 800, 600, Color::create(0.9, 0.9, 0.9)};
	window.set_child(&background);
	SimpleContainer container {0, 0, 800, 600};
	background.set_child(&container);
	Font font {"/usr/share/fonts/truetype/roboto/hinted/Roboto-Regular.ttf", 16};

	Rectangle rectangle {100, 100, 250, 150, Color::create(0.3, 0.3, 0.5)};
	container.add_child(&rectangle);
	TextContainer rectangle_text {&font, "Rectangle", Color::create(1, 1, 1, 0.9), 250, 150};
	rectangle.set_child(&rectangle_text);

	RoundedRectangle rounded_rectangle {450, 100, 250, 150, Color::create(0.3, 0.3, 0.5), 10};
	container.add_child(&rounded_rectangle);
	TextContainer rounded_rectangle_text {&font, "RoundedRectangle", Color::create(1, 1, 1, 0.9), 250, 150};
	rounded_rectangle.set_child(&rounded_rectangle_text);

	Gradient gradient {100, 350, 250, 150, Color::create(0.4, 0.4, 0.6), Color::create(0.2, 0.2, 0.4)};
	container.add_child(&gradient);
	TextContainer gradient_text {&font, "Gradient", Color::create(1, 1, 1, 0.9), 250, 150};
	gradient.set_child(&gradient_text);

	window.run();
}
