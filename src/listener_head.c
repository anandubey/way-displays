#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wayland-util.h>

#include "listeners.h"

#include "cfg.h"
#include "list.h"
#include "types.h"
#include "wlr-output-management-unstable-v1.h"

// Head data

bool max_preferred_refresh(struct Cfg *cfg, const char *name_desc) {
	for (struct SList *i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
		if (strcasecmp(i->val, name_desc) == 0) {
			return true;
		}
	}
	return false;
}

static void name(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *name) {
	struct Head *head = data;

	head->dirty = true;

	head->name = strdup(name);

	head->max_preferred_refresh |=
		max_preferred_refresh(head->output_manager->displ->cfg, name);
}

static void description(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *description) {
	struct Head *head = data;

	head->dirty = true;

	head->description = strdup(description);

	head->max_preferred_refresh |=
		max_preferred_refresh(head->output_manager->displ->cfg, description);
}

static void physical_size(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t width,
		int32_t height) {
	struct Head *head = data;

	head->dirty = true;

	head->width_mm = width;
	head->height_mm = height;
}

static void mode(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	struct Head *head = data;

	head->dirty = !head->pending.mode;

	struct Mode *mode = calloc(1, sizeof(struct Mode));
	mode->head = head;
	mode->zwlr_mode = zwlr_output_mode_v1;

	slist_append(&head->modes, mode);

	zwlr_output_mode_v1_add_listener(zwlr_output_mode_v1, mode_listener(), mode);
}

static void enabled(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t enabled) {
	struct Head *head = data;

	head->dirty = !head->pending.enabled || enabled == head->lid_closed;

	head->enabled = enabled;
}

static void current_mode(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		struct zwlr_output_mode_v1 *zwlr_output_mode_v1) {
	struct Head *head = data;
	head->dirty = true;

	struct Mode *mode = NULL;
	for (struct SList *i = head->modes; i; i = i->nex) {
		mode = i->val;
		if (mode && mode->zwlr_mode == zwlr_output_mode_v1) {
			head->current_mode = mode;
			break;
		}
	}
}

static void position(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t x,
		int32_t y) {
	struct Head *head = data;
	head->dirty = !head->pending.position;

	head->x = x;
	head->y = y;
}

static void transform(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		int32_t transform) {
	struct Head *head = data;
	head->dirty = true;

	head->transform = transform;
}

static void scale(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		wl_fixed_t scale) {
	struct Head *head = data;
	head->dirty = !head->pending.scale;

	head->scale = scale;
}

static void make(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *make) {
	struct Head *head = data;
	head->dirty = true;

	head->make = strdup(make);
}

static void model(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *model) {
	struct Head *head = data;
	head->dirty = true;

	head->model = strdup(model);
}

static void serial_number(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1,
		const char *serial_number) {
	struct Head *head = data;
	head->dirty = true;

	head->serial_number = strdup(serial_number);
}

static void finished(void *data,
		struct zwlr_output_head_v1 *zwlr_output_head_v1) {
	struct Head *head = data;

	// dummy Head, just for printing
	if (head->output_manager) {
		struct Head *head_departed = calloc(1, sizeof(struct Head));

		head_departed->name = strdup(head->name);
		head_departed->description = strdup(head->description);

		slist_append(&head->output_manager->heads_departed, head_departed);
	}

	head->output_manager->dirty = true;

	slist_remove_all(&head->output_manager->desired.heads, NULL, head);
	slist_remove_all(&head->output_manager->heads_arrived, NULL, head);
	slist_remove_all(&head->output_manager->heads_departed, NULL, head);
	slist_remove_all(&head->output_manager->heads, NULL, head);

	free_head(head);

	zwlr_output_head_v1_destroy(zwlr_output_head_v1);
}

static const struct zwlr_output_head_v1_listener listener = {
	.name = name,
	.description = description,
	.physical_size = physical_size,
	.mode = mode,
	.enabled = enabled,
	.current_mode = current_mode,
	.position = position,
	.transform = transform,
	.scale = scale,
	.serial_number = serial_number,
	.model = model,
	.make = make,
	.finished = finished,
};

const struct zwlr_output_head_v1_listener *head_listener() {
	return &listener;
}

