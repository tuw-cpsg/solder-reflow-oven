/**
 * @file pid.c
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief Simple PID controller
 */
#include "pid.h"

pid_state_t pid_init(int16_t P, int16_t I, int16_t D)
{
    pid_state_t state = { .P = P, .I = I, .D = D, .last_diff = 0, .integ = 0 };
    return state;
}

int32_t pid_update(pid_state_t *state, int16_t diff)
{
    int16_t deriv_diff = diff - state->last_diff;
    state->last_diff = diff;
    state->integ += diff;

    int32_t sum = ((int32_t)state->P) * ((int32_t)diff);
    sum += ((int32_t)state->I) * ((int32_t)state->integ);
    sum += ((int32_t)state->D) * ((int32_t)deriv_diff);
    return sum;
}
