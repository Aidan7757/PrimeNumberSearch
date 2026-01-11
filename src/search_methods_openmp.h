#pragma once
#include <stdbool.h>
#include "../src/models.h"

bool naive_check(long potential_prime, struct Config* config);

bool miller_rabin(long potential_prime, const struct Config* config);

