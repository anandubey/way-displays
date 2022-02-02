#include <stdbool.h>
#include <stdlib.h>

#include "types.h"

#include "list.h"

void free_mode(struct Mode *mode) {
	if (!mode)
		return;

	free(mode);
}

void free_head(struct Head *head) {
	if (!head)
		return;

	for (struct SList *i = head->modes; i; i = i->nex) {
		free_mode(i->val);
	}
	slist_free(&head->modes);

	free(head->name);
	free(head->description);
	free(head->make);
	free(head->model);
	free(head->serial_number);

	free(head);
}

void free_output_manager(struct OutputManager *output_manager) {
	if (!output_manager)
		return;

	for (struct SList *i = output_manager->heads; i; i = i->nex) {
		free_head(i->val);
	}
	slist_free(&output_manager->heads);
	slist_free(&output_manager->desired.heads);

	free(output_manager->interface);

	output_manager_free_heads_departed(output_manager);

	free(output_manager);
}

void free_displ(struct Displ *displ) {
	if (!displ)
		return;

	free_output_manager(displ->output_manager);

	free_cfg(displ->cfg);

	free_lid(displ->lid);

	free(displ);
}

void free_user_scale(struct UserScale *user_scale) {
	if (!user_scale)
		return;

	free(user_scale->name_desc);

	free(user_scale);
}

void free_cfg(struct Cfg *cfg) {
	if (!cfg)
		return;

	free(cfg->dir_path);
	free(cfg->file_path);
	free(cfg->file_name);

	if (cfg->auto_scale) {
		free(cfg->auto_scale);
	}
	if (cfg->arrange) {
		free(cfg->arrange);
	}
	if (cfg->align) {
		free(cfg->align);
	}

	for (struct SList *i = cfg->order_name_desc; i; i = i->nex) {
		free(i->val);
	}
	slist_free(&cfg->order_name_desc);

	for (struct SList *i = cfg->user_scales; i; i = i->nex) {
		free_user_scale(i->val);
	}
	slist_free(&cfg->user_scales);

	for (struct SList *i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
		free(i->val);
	}
	slist_free(&cfg->max_preferred_refresh_name_desc);

	for (struct SList *i = cfg->disabled_name_desc; i; i = i->nex) {
		free(i->val);
	}
	slist_free(&cfg->disabled_name_desc);

	if (cfg->laptop_display_prefix) {
		free(cfg->laptop_display_prefix);
	}

	free(cfg);
}

void free_lid(struct Lid *lid) {
	if (!lid)
		return;

	free(lid->device_path);

	free(lid);
}

void head_free_mode(struct Head *head, struct Mode *mode) {
	if (!head || !mode)
		return;

	head->dirty = true;

	if (head->desired.mode == mode) {
		head->desired.mode = NULL;
	}
	if (head->current_mode == mode) {
		head->current_mode = NULL;
	}

	slist_remove_all(&head->modes, mode);

	free_mode(mode);
}

void output_manager_free_head(struct OutputManager *output_manager, struct Head *head) {
	if (!output_manager || !head)
		return;

	output_manager->dirty = true;

	slist_remove_all(&output_manager->desired.heads, head);
	slist_remove_all(&output_manager->heads, head);

	free_head(head);
}

void output_manager_free_heads_departed(struct OutputManager *output_manager) {
	struct SList *i, *r;
	struct Head *head;

	if (!output_manager)
		return;

	i = output_manager->heads_departed;
	while(i) {
		head = i->val;
		r = i;
		i = i->nex;

		slist_remove(&output_manager->heads_departed, &r);
		free_head(head);
	}
}

bool is_dirty(struct Displ *displ) {
	struct SList *i;
	struct Head *head;

	if (!displ)
		return false;

	if (displ->cfg && displ->cfg->dirty)
		return true;

	if (displ->lid && displ->lid->dirty)
		return true;

	if (!displ->output_manager)
		return false;

	if (displ->output_manager->dirty) {
		return true;
	}

	for (i = displ->output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		if (head->dirty) {
			return true;
		}
	}

	return false;
}

void reset_dirty(struct Displ *displ) {
	struct SList *i;
	struct Head *head;

	if (!displ)
		return;

	if (displ->cfg)
		displ->cfg->dirty = false;

	if (displ->lid)
		displ->lid->dirty = false;

	if (!displ->output_manager)
		return;

	displ->output_manager->dirty = false;

	for (i = displ->output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		head->dirty = false;
	}
}

bool is_pending_output_manager(struct OutputManager *output_manager) {
	struct SList *i;
	struct Head *head;

	if (!output_manager)
		return false;

	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;

		if (is_pending_head(head)) {
			return true;
		}
	}

	return false;
}

bool is_pending_head(struct Head *head) {
	return (head &&
			(head->pending.mode ||
			 head->pending.scale ||
			 head->pending.enabled ||
			 head->pending.position));
}

void reset_pending_desired(struct OutputManager *output_manager) {
	struct SList *i;
	struct Head *head;

	if (!output_manager)
		return;

	slist_free(&output_manager->desired.heads);

	for (i = output_manager->heads; i; i = i->nex) {
		head = i->val;
		if (!head)
			continue;

		head->pending.mode = false;
		head->pending.scale = false;
		head->pending.enabled = false;
		head->pending.position = false;

		head->desired.mode = NULL;
		head->desired.scale = 0;
		head->desired.enabled = false;
		head->desired.x = 0;
		head->desired.y = 0;
	}
}

bool get_auto_scale(struct Cfg *cfg) {
	if (cfg && cfg->auto_scale) {
		return *cfg->auto_scale;
	} else {
		return AutoScaleDefault;
	}
}

void set_auto_scale(struct Cfg *cfg, bool auto_scale) {
	if (!cfg) {
		return;
	}
	if (!cfg->auto_scale) {
		cfg->auto_scale = (bool*)calloc(1, sizeof(bool));
	}
	*cfg->auto_scale = auto_scale;
}

enum Arrange get_arrange(struct Cfg *cfg) {
	if (cfg && cfg->arrange) {
		return *cfg->arrange;
	} else {
		return ArrangeDefault;
	}
}

void set_arrange(struct Cfg *cfg, enum Arrange arrange) {
	if (!cfg) {
		return;
	}
	if (!cfg->arrange) {
		cfg->arrange = (enum Arrange*)calloc(1, sizeof(enum Arrange));
	}
	*cfg->arrange = arrange;
}

enum Align get_align(struct Cfg *cfg) {
	if (cfg && cfg->align) {
		return *cfg->align;
	} else {
		return AlignDefault;
	}
}

void set_align(struct Cfg *cfg, enum Align align) {
	if (!cfg) {
		return;
	}
	if (!cfg->align) {
		cfg->align = (enum Align*)calloc(1, sizeof(enum Align));
	}
	*cfg->align = align;
}

