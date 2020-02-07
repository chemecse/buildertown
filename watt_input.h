#ifndef WATT_INPUT
#define WATT_INPUT

#include <stdint.h>

struct input_button_state {
	int32_t is_down;
	int32_t was_down;
};

struct input {
	struct input_button_state up;
	struct input_button_state down;
	struct input_button_state left;
	struct input_button_state right;
	struct input_button_state quit;
	struct input_button_state rise;
	struct input_button_state fall;
	struct input_button_state look_up;
	struct input_button_state look_down;
	struct input_button_state action;
	struct input_button_state lmb;
};

void input_button_process(struct input_button_state *button, int32_t is_down);

#endif
