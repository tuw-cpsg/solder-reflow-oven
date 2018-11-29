/**
 * @file reflow.h
 * @author Michael Platzer
 * @date 2018-11-29
 *
 * @brief Manage the process of reflow soldering a circuit board
 */

#ifndef REFLOW_H
#define REFLOW_H

/**
 * @brief Start reflowing.
 *
 * This function initializes the reflow process manager. It is used to reset
 * the reflow process manager and must be called each time a reflow process
 * is started.
 */
void reflow_start();

/**
 * @brief Update the reflow process state.
 *
 * This function must be called every second while the reflow process is
 * running. @p temp is the temperature within the oven (i.e. the
 * temperature of the hot junction of the thermocouple).
 *
 * The function returns the output value for driving the heater elements.
 *
 * The reflow process has ended if the output value is 0 and the
 * temperature has dropped below 50 degC.
 */
int reflow_update(int temp);

#endif // REFLOW_H
