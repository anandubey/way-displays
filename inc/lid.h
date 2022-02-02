#ifndef LID_H
#define LID_H

#include <stdbool.h>

#include "types.h"

struct Lid *create_lid();

void destroy_lid(struct Displ *displ);

bool update_lid(struct Displ *displ);

void update_heads_lid_closed(struct Displ *displ);

#endif // LID_H

