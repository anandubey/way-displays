#include <stdlib.h>

#include "mode.h"

#include "log.h"
#include "types.h"

int32_t mhz_to_hz(int32_t mhz) {
	return (mhz + 500) / 1000;
}

bool equal_res_hz(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct Mode *lhs = (struct Mode*)a;
	struct Mode *rhs = (struct Mode*)b;

	return lhs->width == rhs->width &&
		lhs->height == rhs->height &&
		mhz_to_hz(lhs->refresh_mhz) == mhz_to_hz(rhs->refresh_mhz);
}

bool greater_than_res_refresh(const void *a, const void *b) {
	if (!a || !b) {
		return false;
	}

	struct Mode *lhs = (struct Mode*)a;
	struct Mode *rhs = (struct Mode*)b;

	if (lhs->width > rhs->width) {
		return true;
	} else if (lhs->width != rhs->width) {
		return false;
	}

	if (lhs->height > rhs->height) {
		return true;
	} else if (lhs->height != rhs->height) {
		return false;
	}

	if (lhs->refresh_mhz > rhs->refresh_mhz) {
		return true;
	}

	return false;
}

double mode_dpi(struct Mode *mode) {
	if (!mode || !mode->head || !mode->head->width_mm || !mode->head->height_mm) {
		return 0;
	}

	double dpi_horiz = (double)(mode->width) / mode->head->width_mm * 25.4;
	double dpi_vert = (double)(mode->height) / mode->head->height_mm * 25.4;
	return (dpi_horiz + dpi_vert) / 2;
}

struct SList *modes_res_refresh(struct SList *modes) {
	struct SList *mrrs = NULL;

	struct SList *sorted = slist_sort(modes, greater_than_res_refresh);

	struct ModesResRefresh *mrr = NULL;
	struct Mode *mode = NULL;
	for (struct SList *i = sorted; i; i = i->nex) {
		mode = i->val;

		if (!mrr || !equal_res_hz(mode, mrr->modes->val)) {
			mrr = calloc(1, sizeof(struct ModesResRefresh));
			mrr->width = mode->width;
			mrr->height = mode->height;
			mrr->refresh_hz = mhz_to_hz(mode->refresh_mhz);
			slist_append(&mrrs, mrr);
		}

		slist_append(&mrr->modes, mode);
	}

	slist_free(&sorted);

	return mrrs;
}

struct Mode *mode_optimal(struct SList *modes, bool max_preferred_refresh) {
	struct Mode *mode, *mode_optimal, *preferred_mode;

	mode_optimal = NULL;
	preferred_mode = NULL;
	for (struct SList *i = modes; i; i = i->nex) {
		mode = i->val;

		if (!mode) {
			continue;
		}

		if (!mode_optimal) {
			mode_optimal = mode;
		}

		// preferred first
		if (mode->preferred) {
			mode_optimal = mode;
			preferred_mode = mode;
			break;
		}

		// highest resolution
		if (mode->width * mode->height > mode_optimal->width * mode_optimal->height) {
			mode_optimal = mode;
			continue;
		}

		// highest refresh at highest resolution
		if (mode->width == mode_optimal->width &&
				mode->height == mode_optimal->height &&
				mode->refresh_mhz > mode_optimal->refresh_mhz) {
			mode_optimal = mode;
			continue;
		}
	}

	if (preferred_mode && max_preferred_refresh) {
		mode_optimal = preferred_mode;
		for (struct SList *i = modes; i; i = i->nex) {
			mode = i->val;
			if (mode->width == mode_optimal->width && mode->height == mode_optimal->height) {
				if (mode->refresh_mhz > mode_optimal->refresh_mhz) {
					mode_optimal = mode;
				}
			}
		}
	}

	return mode_optimal;
}

