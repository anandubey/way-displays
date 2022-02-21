#ifndef CFG_H
#define CFG_H

#ifndef __cplusplus

#include <stdbool.h>

#else

#include <yaml-cpp/emitter.h>
#include <yaml-cpp/node/node.h>

extern "C" { //}

#endif

struct UserScale {
	char *name_desc;
	float scale;
};

enum Arrange {
	ROW = 1,
	COL,
};
extern enum Arrange ARRANGE_DEFAULT;

enum Align {
	TOP = 1,
	MIDDLE,
	BOTTOM,
	LEFT,
	RIGHT,
};
extern enum Align ALIGN_DEFAULT;

enum AutoScale {
	ON = 1,
	OFF,
};
extern enum AutoScale AUTO_SCALE_DEFAULT;

struct Cfg {
	char *dir_path;
	char *file_path;
	char *file_name;

	bool dirty;
	bool written;

	char *laptop_display_prefix;
	struct SList *order_name_desc;
	enum Arrange arrange;
	enum Align align;
	enum AutoScale auto_scale;
	struct SList *user_scales;
	struct SList *max_preferred_refresh_name_desc;
	struct SList *disabled_name_desc;
};

extern const char *LAPTOP_DISPLAY_PREFIX_DEFAULT;

enum CfgElement {
	ARRANGE = 1,
	ALIGN,
	ORDER,
	AUTO_SCALE,
	SCALE,
	LAPTOP_DISPLAY_PREFIX,
	MAX_PREFERRED_REFRESH,
	LOG_THRESHOLD,
	DISABLED,
	ARRANGE_ALIGN,
};

enum CfgMergeType {
	SET = 1,
	DEL,
};

struct Cfg *cfg_clone(struct Cfg *from);

struct Cfg *cfg_file_load();

struct Cfg *cfg_file_reload(struct Cfg *cfg);

void cfg_file_write(struct Cfg *cfg);

void cfg_fix(struct Cfg *cfg);

struct Cfg *cfg_merge(struct Cfg *cfg_to, struct Cfg *cfg_from, enum CfgMergeType merge_type);

void free_user_scale(void *user_scale);

void free_cfg(struct Cfg *cfg);

#if __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus

void cfg_emit(YAML::Emitter &e, struct Cfg *cfg);

void cfg_parse_node(struct Cfg *cfg, YAML::Node &node);

#endif

#endif // CFG_H

