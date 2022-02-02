#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

struct Mode {
	struct Head *head;

	struct zwlr_output_mode_v1 *zwlr_mode;

	int32_t width;
	int32_t height;
	int32_t refresh_mHz;
	bool preferred;
};

struct Head {
	struct OutputManager *output_manager;

	struct zwlr_output_head_v1 *zwlr_head;

	struct zwlr_output_configuration_head_v1 *zwlr_config_head;

	struct SList *modes;

	bool dirty;

	char *name;
	char *description;
	int32_t width_mm;
	int32_t height_mm;
	int enabled;
	struct Mode *current_mode;
	struct Mode *preferred_mode;
	int32_t x;
	int32_t y;
	enum wl_output_transform transform;
	wl_fixed_t scale;
	char *make;
	char *model;
	char *serial_number;
	bool lid_closed;
	bool max_preferred_refresh;

	struct {
		struct Mode *mode;
		wl_fixed_t scale;
		int enabled;
		// layout coords
		int32_t x;
		int32_t y;
		int32_t width;
		int32_t height;
	} desired;

	struct {
		bool mode;
		bool scale;
		bool enabled;
		bool position;
	} pending;
};

struct OutputManager {
	struct Displ *displ;

	struct zwlr_output_manager_v1 *zwlr_output_manager;

	struct SList *heads;

	bool dirty;

	uint32_t serial;
	char *interface;
	struct SList *heads_arrived;
	struct SList *heads_departed;

	int retries;

	struct {
		struct SList *heads;
	} desired;
};

struct Displ {
	struct wl_display *display;

	struct wl_registry *registry;

	struct OutputManager *output_manager;
	struct Cfg *cfg;
	struct Lid *lid;

	uint32_t name;
};

struct UserScale {
	char *name_desc;
	float scale;
};

enum Arrange {
	ROW,
	COL,
};

enum Align {
	TOP,
	MIDDLE,
	BOTTOM,
	LEFT,
	RIGHT,
};



struct Cfg {
	char *dir_path;
	char *file_path;
	char *file_name;

	bool dirty;

	char *laptop_display_prefix;
	struct SList *order_name_desc;
	enum Arrange *arrange;
	enum Align *align;
	bool *auto_scale;
	struct SList *user_scales;
	struct SList *max_preferred_refresh_name_desc;
	struct SList *disabled_name_desc;
};

#define ArrangeDefault ROW
#define AlignDefault TOP
#define AutoScaleDefault true
#define LaptopDisplayPrefixDefault "eDP"

struct Lid {
	bool closed;

	bool dirty;

	char *device_path;
	struct libinput *libinput_monitor;
	int libinput_fd;
};

void free_mode(struct Mode *mode);
void free_head(struct Head *head);
void free_output_manager(struct OutputManager *output_manager);
void free_displ(struct Displ *displ);
void free_user_scale(struct UserScale *user_scale);
void free_cfg(struct Cfg *cfg);
void free_lid(struct Lid *lid);

void head_free_mode(struct Head *head, struct Mode *mode);
void output_manager_free_head(struct OutputManager *output_manager, struct Head *head);
void output_manager_free_heads_departed(struct OutputManager *output_manager);

bool is_dirty(struct Displ *displ);
void reset_dirty(struct Displ *displ);

bool is_pending_output_manager(struct OutputManager *output_manager);
bool is_pending_head(struct Head *head);

void reset_pending_desired(struct OutputManager *output_manager);

bool get_auto_scale(struct Cfg *cfg);
void set_auto_scale(struct Cfg *cfg, bool auto_scale);
enum Arrange get_arrange(struct Cfg *cfg);
void set_arrange(struct Cfg *cfg, enum Arrange arrange);
enum Align get_align(struct Cfg *cfg);
void set_align(struct Cfg *cfg, enum Align align);

#endif // TYPES_H

