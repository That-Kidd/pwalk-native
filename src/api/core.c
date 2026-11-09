#include <string.h>
#include "pw_internal.h"

void pw_set_hal(pw_state_t *s, const pw_hal_t *hal) {
    s->hal = hal;
}
