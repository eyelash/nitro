#include <nitro.hpp>
using namespace nitro;

class DemoWidget: public BlurredRectangle {
	RoundedRectangle rounded_rectangle;
	RoundedBorder border;
	TextContainer text;
	Animator<Color> animator;
public:
	DemoWidget(FontSet* font):
		BlurredRectangle(Color::create(0, 0, 0, 0.2), 8, 20),
		rounded_rectangle(Color::create(0.2, 0.9, 0.7), 10),
		border(2, Color::create(0, 0, 0, 0.2), 10),
		text(font, "some text", Color::create(0, 0, 0, 0.5)),
		animator(rounded_rectangle.color())
	{
		set_child(&rounded_rectangle);
		rounded_rectangle.set_child(&border);
		border.set_child(&text);
	}
	void mouse_enter() override {
		BlurredRectangle::mouse_enter();
		animator.animate(Color::create(0.5, 0.9, 0.4), 200);
	}
	void mouse_leave() override {
		BlurredRectangle::mouse_leave();
		animator.animate(Color::create(0.2, 0.9, 0.7), 200);
	}
};

int main() {
	Window window(800, 500, "demo");
	FontSet font("Roboto", 16);

	Rectangle background(Color::create(0.9, 0.9, 0.9));
	window.set_child(&background);
	Padding padding(100);
	background.set_child(&padding);

	DemoWidget widget(&font);
	padding.set_child(&widget);

	window.run();
}
