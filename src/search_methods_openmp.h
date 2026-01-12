#pragma once
#include <stdbool.h>
#include "../src/models.h"

bool naive_check(long long potential_prime, struct Config* config);

bool miller_rabin(long long potential_prime, const struct Config* config);

bool fermat(long long potential_prime, const struct Config* config);

bool gauss_euler(long long potential_prime);

bool mr_ge(long long potential_prime);

