#include "watt_input.h"

void input_button_process(struct input_button_state *button, int32_t is_down)
{
	button->was_down = (button->is_down && !is_down);
	button->is_down = is_down;
	return;
}
