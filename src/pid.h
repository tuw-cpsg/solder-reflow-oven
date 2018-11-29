/**
 * @file pid.h
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief Simple PID controller
 */

#ifndef PID_H
#define PID_H

#include <stdint.h>

typedef struct {
    int16_t P, I, D;
    int16_t last_diff, integ;
} pid_state_t;

/**
 * @brief Initialize a PID controller.
 *
 * This function initializes a PID controller with the specified gain
 * parameters: @p P for the proportional gain, @p I for the integral gain
 * and @p D for the differential gain.
 *
 * The function returns a PID state type, which has been initialized
 * accordingly.
 */
pid_state_t pid_init(int16_t P, int16_t I, int16_t D);

/**
 * @brief Update the state of a PID controller with a new input.
 *
 * This function updates the state of a PID controller with a new input
 * value. @p state is a pointer to the PID state type of the controller
 * and @p diff is the difference between the actual and the target value
 * of the system.
 *
 * The function updates the state of the PID controller and returns the
 * new output value of the controller.
 */
int32_t pid_update(pid_state_t *state, int16_t diff);

#endif // PID_H
