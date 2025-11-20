//
// Created by jmc3292 on 11/6/2025.
//

#include "arduino_link.h" // This is the header file you provided
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// --- API Error Codes ---
// Copied from your "arduino_link.cpp" file.
// Since these are not in "arduino_link.h", the tester program
// must define them to understand the function results.
#define API_SUCCESS 0
#define API_ERROR_BAD_HANDLE -1
#define API_ERROR_SERIAL_FAIL -2
#define API_ERROR_BAD_TIME_STR -3
#define API_ERROR_OPEN_FAIL -4   // Note: Your .cpp file doesn't seem to return this
#define API_ERROR_BAD_POWER -5   // Note: Your .cpp file doesn't seem to return this
#define API_ERROR_ARDUINO_ERR -6
#define API_ERROR_UNKNOWN -7

// Helper function to translate error codes into human-readable strings
std::string get_error_string(int32_t code) {
    switch (code) {
        case API_SUCCESS:           return "API_SUCCESS";
        case API_ERROR_BAD_HANDLE:  return "API_ERROR_BAD_HANDLE";
        case API_ERROR_SERIAL_FAIL: return "API_ERROR_SERIAL_FAIL";
        case API_ERROR_BAD_TIME_STR:return "API_ERROR_BAD_TIME_STR";
        case API_ERROR_OPEN_FAIL:   return "API_ERROR_OPEN_FAIL";
        case API_ERROR_BAD_POWER:   return "API_ERROR_BAD_POWER";
        case API_ERROR_ARDUINO_ERR: return "API_ERROR_ARDUINO_ERR";
        case API_ERROR_UNKNOWN:     return "API_ERROR_UNKNOWN";
        default:                    return "Unknown Error Code";
    }
}

/**
 * @brief Checks the result of an API call and prints a message.
 * @return true on success, false on failure.
 */
bool check_result(int32_t result, const std::string& action_name) {
    // Your header comments and implementation agree: 0 means SUCCESS.
    if (result == API_SUCCESS) {
        std::cout << "[SUCCESS] " << action_name << std::endl;
        return true;
    } else {
        std::cerr << "[FAILED]  " << action_name << " - Error: "
                  << get_error_string(result) << " (" << result << ")" << std::endl;
        return false;
    }
}

int main() {
    std::string port_name;
    uint32_t baud_rate = 115200; // Default baud

    std::cout << "--- Microwave DLL Tester ---" << std::endl;
    std::cout << "Enter COM port (e.g., COM3 on Windows, /dev/ttyUSB0 on Linux): ";
    std::getline(std::cin, port_name);

    std::cout << "Enter baud rate (default is 115200, press Enter to use): ";
    std::string baud_str;
    std::getline(std::cin, baud_str);
    if (!baud_str.empty()) {
        try {
            baud_rate = std::stoul(baud_str);
        } catch (...) {
            std::cerr << "Invalid input, using default 115200." << std::endl;
        }
    }

    // --- 1. Open Connection ---
    std::cout << "\nAttempting to open " << port_name << " at " << baud_rate << " baud..." << std::endl;

    // Call the DLL function
    MicrowaveHandle handle = open_microwave_controller(port_name.c_str(), baud_rate);

    // Your header says: returns 0 on failure.
    if (handle == 0) {
        std::cerr << "[FATAL] Failed to open microwave controller (open_microwave_controller returned 0)." << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return -1;
    }

    std::cout << "[SUCCESS] Connection open. Handle: " << (void*)handle << std::endl;
    std::cout << "Waiting for Arduino to settle (2s wait is in the DLL)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Give a little extra time

    // --- 2. Test Sequence ---
    int32_t result;

    std::cout << "\n--- Test 1: Run for 5s at 100% power ---" << std::endl;
    result = run_microwave(handle, "00:05", 100);
    if (!check_result(result, "run_microwave(\"00:05\", 100)")) {
        goto cleanup; // On failure, jump to close
    }
    std::cout << "Waiting 6 seconds (letting it run)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(6));
    result = send_microwave_command(handle, "press stop");
    check_result(result, "send_microwave_command(\"press stop\")");

    std::cout << "\n--- Test 2: Run for 10s at 50% power ---" << std::endl;
    result = run_microwave(handle, "00:10", 50);
    if (!check_result(result, "run_microwave(\"00:10\", 50)")) {
        goto cleanup;
    }
    std::cout << "Waiting 5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    result = send_microwave_command(handle, "press stop");
    check_result(result, "send_microwave_command(\"press stop\")");

    std::cout << "\n--- Test 3: Stop mid-cook ---" << std::endl;
    result = stop_microwave(handle);
    check_result(result, "stop_microwave()");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n--- Test 4: Invalid Time String ('1:30' instead of '01:30') ---" << std::endl;
    result = run_microwave(handle, "1:30", 100);
    check_result(result, "run_microwave(\"1:30\", 100)");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (result != API_ERROR_BAD_TIME_STR) {
        std::cerr << "   > Note: Expected API_ERROR_BAD_TIME_STR!" << std::endl;
    }
    for (int i = 0; i < 2; i++) {
        result = send_microwave_command(handle, "press stop");
        check_result(result, "send_microwave_command(\"press stop\")");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n--- Test 5: Raw Command ('press 1') ---" << std::endl;
    result = send_microwave_command(handle, "press 1");
    check_result(result, "send_microwave_command(\"press 1\")");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Clear the "1" we just pressed
    result = send_microwave_command(handle, "press stop");
    check_result(result, "send_microwave_command(\"press stop\")");
    stop_microwave(handle);


cleanup:
    // --- 3. Close Connection ---
    std::cout << "\n--- Test Complete: Closing connection ---" << std::endl;
    result = close_microwave_controller(handle);
    // Note: Your header comment for close_microwave_controller is incorrect.
    // The *implementation* correctly returns 0 (API_SUCCESS) on success.
    // This check_result function correctly checks for 0.
    check_result(result, "close_microwave_controller()");

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}