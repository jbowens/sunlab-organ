#pragma once

/*
 * A struct that represents the state of the organ. The server broadcasts
 * new copies of this struct whenever the state of the organ changes.
 *
 */
typedef struct {
    bool flags[26];
} organ_state;

