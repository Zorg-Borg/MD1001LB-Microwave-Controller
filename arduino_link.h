//
// Created by jack on 11/5/2025.
//
#ifndef MD1001LB_MICROWAVE_CONTROLLER_ARDUINO_LINK_H
#define MD1001LB_MICROWAVE_CONTROLLER_ARDUINO_LINK_H

#include <stdint.h> // For standard integer types

// Define DLL_EXPORT for Windows compilation
#ifdef _WIN32
    #ifdef BUILDING_DLL
        #define DLL_EXPORT __declspec(dllexport)
    #else
        #define DLL_EXPORT __declspec(dllimport)
    #endif
#else
    #define DLL_EXPORT
#endif

// Use extern "C" to ensure C-style function names
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @breif Opaque handle to a microwave controller instance.
 * Using intptr_t so labVIEW can treat it as a 64-bit (or 32-bit) systems.
 */
    typedef intptr_t MicrowaveHandle;
/**
 * @breif  Opens a serial connection to the Arduino
 *
 * @param port_name The name of the serial port (e.g., "COM3" on Windows or "/dev/ttyUSB0" on Linux).
 * @param baud_rate The baud rate for the serial communication (e.g., 9600 or 115200).
 *
 * @return a non-zero MicrowaveHandle on success, or 0 on failure.
 */
    DLL_EXPORT MicrowaveHandle open_microwave_controller(const char* port_name, uint32_t baud_rate);

/**
 * @breif Closes the serial connection to the Arduino.
 *
 * @param handle The handle to the microwave controller instance.
 *
 * @return A non-zero value on success, or 0 on failure.
 */
    DLL_EXPORT int32_t close_microwave_controller(MicrowaveHandle handle);

/**
 * @brief sends one command over serial to the microwave controller (Ex. start, stop, set power, 1, 2, 3 etc.).
 *
 * @param handle The handle to the microwave controller instance.
 * @param command The command string to send.
 *
 * @return 0 on success, non-zero on failure.
 */
    DLL_EXPORT int32_t send_microwave_command(MicrowaveHandle handle, const char* command);

/**
 * @name run
 *
 * @brief runs the microwave for a specified duration at a specified power level.
 *
 * @param handle The handle to the microwave controller instance.
 * @param time_str The time duration string (e.g., "01:30" for 1 minute and 30 seconds).
 * @param power_level The power level a percentage (0-100) counted by 10s (e.g., 10, 20, ..., 100).
 * Defaults to 100 if no value is given or invalid value is given.
 *
 * @return 0 on success, non-zero on failure.
 */
    DLL_EXPORT int32_t run_microwave(MicrowaveHandle handle, const char* time_str, uint8_t power_level);

/**
 * @name stop
 *
 * @brief stops the microwave operation.
 *
 * @return 0 on success, non-zero on failure.
 */
    DLL_EXPORT int32_t stop_microwave(MicrowaveHandle handle);

}
#endif //MD1001LB_MICROWAVE_CONTROLLER_ARDUINO_LINK_H