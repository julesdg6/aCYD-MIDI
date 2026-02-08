#include "common_definitions.h"

Scale scales[] = {
    {"Major", {0, 2, 4, 5, 7, 9, 11}, 7},
    {"Minor", {0, 2, 3, 5, 7, 8, 10}, 7},
    {"Dorian", {0, 2, 3, 5, 7, 9, 10}, 7},
    {"Penta", {0, 2, 4, 7, 9}, 5},
    {"Blues", {0, 3, 5, 6, 7, 10}, 6},
    {"Chrome", {0, 1, 2, 3, 4, 5, 6, 7}, 8},
};

const int NUM_SCALES = sizeof(scales) / sizeof(scales[0]);
