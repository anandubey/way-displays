#include <stdbool.h>
#include <stddef.h>

#include "listeners.h"

#include "list.h"
#include "log.h"
#include "types.h"
#include "wlr-output-management-unstable-v1.h"

#define MAX_RETRIES 3

// OutputManager data

static void succeeded(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	struct OutputManager *output_manager = data;

	reset_pending_desired(output_manager);
	output_manager->retries = 0;

	log_info("\nChanges successful");

	for (struct SList *i = output_manager->heads; i; i = i->nex) {
		struct Head *head = i->val;
		if (head->zwlr_config_head) {
			zwlr_output_configuration_head_v1_destroy(head->zwlr_config_head);
			head->zwlr_config_head = NULL;
		}
	}
	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);
}

static void failed(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	struct OutputManager *output_manager = data;

	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);

	if (++output_manager->retries > MAX_RETRIES) {
		log_error("\nToo many retries, abandoning changes", __FILE__, __LINE__);
		output_manager->dirty = false;
	} else {
		log_info("\nChanges failed, retrying %d/%d", output_manager->retries, MAX_RETRIES);
		// try again with new state
		output_manager->dirty = true;
	}

	reset_pending_desired(output_manager);
}

static void cancelled(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	struct OutputManager *output_manager = data;

	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);

	if (++output_manager->retries > MAX_RETRIES) {
		log_error("\nToo many retries, abandoning changes", __FILE__, __LINE__);
		output_manager->dirty = false;
	} else {
		log_info("\nChanges cancelled, retrying %d/%d", output_manager->retries, MAX_RETRIES);
		// try again with new state
		output_manager->dirty = true;
	}

	reset_pending_desired(output_manager);
}

static const struct zwlr_output_configuration_v1_listener listener = {
	.succeeded = succeeded,
	.failed = failed,
	.cancelled = cancelled,
};

const struct zwlr_output_configuration_v1_listener *output_configuration_listener() {
	return &listener;
}

