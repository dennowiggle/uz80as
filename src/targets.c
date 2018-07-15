/* ===========================================================================
 * uz80as, an assembler for the Zilog Z80 and several other microprocessors.
 *
 * Target list.
 * ===========================================================================
 */

#include "targets.h"
#include "uz80as.h"

#ifndef STRING_H
#include <string.h>
#endif

extern const struct target s_target_z80;
extern const struct target s_target_gbcpu;
extern const struct target s_target_i8080;

static const struct target *s_targets[] = {
	&s_target_z80,
	&s_target_gbcpu,
	&s_target_i8080,
	NULL,
};

static int s_index;

const struct target *find_target(const char *id)
{
	const struct target **p;

	for (p = s_targets; *p != NULL; p++) {
		if (strcmp(id, (*p)->id) == 0) {
			return *p;
		}
	}

	return NULL;
}

const struct target *first_target(void)
{
	s_index = 0;
	return next_target();
}

const struct target *next_target(void)
{
	if (s_targets[s_index] != NULL) {
		return s_targets[s_index++];
	} else {
		return NULL;
	}
}
