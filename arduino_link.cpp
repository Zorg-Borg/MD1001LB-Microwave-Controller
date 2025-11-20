//
// Created by jack on 11/5/2025.
//
// dll functions implementations

#include "arduino_link.h"

#include <iostream>
#include <istream>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <functional>

#define ASIO_STANDALONE
#include "lib/asio/include/asio.hpp"



// internal error codes
#define API_SUCCESS 0
#define API_ERROR_BAD_HANDLE -1
#define API_ERROR_SERIAL_FAIL -2
#define API_ERROR_BAD_TIME_STR -3
#define API_ERROR_OPEN_FAIL -4
#define API_ERROR_BAD_POWER -5
#define API_ERROR_ARDUINO_ERR -6
#define API_ERROR_UNKNOWN -7

//internal Session object
//this is what MicrowaveHandle will point to
struct MicrowaveSession {
    asio::io_context io;
    asio::serial_port port;

    MicrowaveSession() : io(), port(io) {}
};

// --- Internal Helper Functions ---

/**
 * @brief The core function to send any raw command string to the Arduino.
 *
 * This function blocks until the Arduino has fully acknowledged the
 * command (including waiting for the *second* "OK" on a 'press' command).
 *
 * @param session The active session pointer.
 * @param full_command The complete string to send (e.g., "press 1").
 * @return API_SUCCESS on success, error code on failure.
 */
static int32_t send_raw_command(MicrowaveSession* session, const std::string& full_command) {
    if (!session || !session->port.is_open()) {
        return API_ERROR_BAD_HANDLE;
    }

    try {
        // Send the command with a newline
        asio::write(session->port, asio::buffer(full_command + "\n"));

        asio::streambuf response_buf;
        std::string response_line;

        // Check if this is a command that sends two "OK" responses
        bool is_press_or_pulse = (full_command.find("press") == 0 ||
                                  full_command.find("pulse") == 0);

        while (true) {
            // Read one line from the serial port (blocks until \n)
            asio::read_until(session->port, response_buf, '\n');
            std::istream is(&response_buf);
            std::getline(is, response_line);

            // Clean up trailing \r
            if (!response_line.empty() && response_line.back() == '\r') {
                response_line.pop_back();
            }
            if (response_line.empty()) {
                continue;
            }

            // Check for an error from the Arduino itself
            // if (response_line.find("ERR:") == 0) {
            //     std::cerr << "Arduino Error: " << response_line  << std::endl;
            //     return API_ERROR_ARDUINO_ERR;
            // }

            if (is_press_or_pulse) {
                // For 'press', we must wait for the *second* "OK".
                // The first is "OK: pressing..."
                // The second is just "OK"
                if (response_line == "OK") {
                    break; // This is the final "OK" we need
                }
            } else {
                // For other commands (e.g., "hold", "status"),
                // the first "OK" or "Status" response is enough.
                if (response_line.find("OK") == 0 || response_line.find("Status:") == 0) {
                    break;
                }
            }
        }
    } catch (const asio::system_error& e) {
        std::cerr << "Serial communication error: " << e.what() << std::endl;
        return API_ERROR_SERIAL_FAIL;
    } catch (const std::exception& e) {
        std::cerr << "Unknown error in send_raw_command: " << e.what() << std::endl;
        return API_ERROR_UNKNOWN;
    }

    // Short delay to let the microwave's own controller process the key press
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    return API_SUCCESS;
}

/**
 * @brief Parses "MM:SS" time string into a string of digits "MMSS".
 * @param time_str Input string (e.g., "01:30")
 * @param out_digits Output string (e.g., "130")
 * @return true on success, false on invalid format.
 */
static bool parse_time_to_digits(const std::string& time_str, std::string& out_digits) {
    try {
        size_t colon_pos = time_str.find(':');
        if (colon_pos == std::string::npos || colon_pos == 0 || colon_pos + 1 == time_str.length()) {
            return false; // Invalid format (e.g., ":30", "1:", "130")
        }

        std::string min_str = time_str.substr(0, colon_pos);
        std::string sec_str = time_str.substr(colon_pos + 1);

        if (sec_str.length() != 2) {
             return false; // Seconds must be two digits (e.g., "05")
        }

        // Test conversion
        int minutes = std::stoi(min_str);
        int seconds = std::stoi(sec_str);

        if (minutes < 0 || seconds < 0 || seconds > 59) {
            return false; // Invalid time range
        }

        // Combine into one string for the keypad
        // e.g., "1" + "30" -> "130"
        // e.g., "01" + "30" -> "0130"
        out_digits = min_str + sec_str;
        return true;

    } catch (...) {
        return false; // stoi failed (e.g., "a:30")
    }
}

// --- C-API Implementation ---

// This block ensures C-style function names
#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT MicrowaveHandle open_microwave_controller(const char* port_name, uint32_t baud_rate) {
    std::string port_str = port_name;

    // On Windows, Asio needs the \\.\ prefix for COM ports
    #ifdef _WIN32
        if (port_str.find("COM") == 0) {
            port_str = "\\\\.\\" + port_str;
        }
    #endif

    // Create a new session object on the heap
    MicrowaveSession* session = new (std::nothrow) MicrowaveSession();
    if (!session) {
        return 0; // Out of memory
    }

    try {
        session->port.open(port_str);
        session->port.set_option(asio::serial_port_base::baud_rate(baud_rate));
        session->port.set_option(asio::serial_port_base::character_size(8));
        session->port.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
        session->port.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));

    } catch (const asio::system_error& e) {
        std::cerr << "Failed to open port " << port_str << ": " << e.what() << std::endl;
        delete session;
        return 0; // Return NULL handle on failure
    }

    // Wait for the Arduino to reset after connection
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Clear any startup text from the Arduino
    try {
        // Drain any startup/banner noise and partial tokens without blocking the app,
        // using async read with two timers: a max-total time and a quiet window.
        auto start = std::chrono::steady_clock::now();
        auto last_data = start;
        const auto max_total = std::chrono::milliseconds(400);
        const auto quiet_window = std::chrono::milliseconds(120);

        asio::steady_timer max_timer(session->io);
        asio::steady_timer quiet_timer(session->io);
        bool done = false;

        // Start the max total timer
        max_timer.expires_after(max_total);
        max_timer.async_wait([&](const asio::error_code& /*ec*/) {
            if (!done) {
                done = true;
                asio::error_code ignore_ec;
                session->port.cancel(ignore_ec);
            }
        });

        // Function to (re)arm the quiet timer (recursive lambda needs std::function)
        std::function<void()> arm_quiet;
        arm_quiet = [&]() {
            quiet_timer.expires_after(quiet_window);
            quiet_timer.async_wait([&](const asio::error_code& /*ec*/) {
                auto now = std::chrono::steady_clock::now();
                if (done) return;
                if (now - last_data >= quiet_window) {
                    done = true;
                    asio::error_code ignore_ec;
                    session->port.cancel(ignore_ec);
                } else {
                    // Not quiet long enough; re-arm
                    arm_quiet();
                }
            });
        };
        arm_quiet();

        // Start continuous async drain
        static char drain_buf[256];
        std::function<void()> do_read;
        do_read = [&]() {
            if (done) return;
            session->port.async_read_some(asio::buffer(drain_buf, sizeof(drain_buf)),
                [&](const asio::error_code& ec, std::size_t n) {
                    if (done) return;
                    if (!ec && n > 0) {
                        last_data = std::chrono::steady_clock::now();
                        // keep draining
                        do_read();
                    } else if (ec == asio::error::operation_aborted) {
                        // canceled due to timers -> finish
                    } else if (ec) {
                        // unexpected error; stop
                        done = true;
                        asio::error_code ignore_ec;
                        session->port.cancel(ignore_ec);
                    } else {
                        // zero bytes without error: schedule another read
                        do_read();
                    }
                });
        };
        do_read();

        session->io.restart();
        session->io.run(); // runs until timers cancel pending read(s)

        // Send a newline to ensure the Arduino parser finalizes any partial token.
        asio::error_code ec;
        asio::write(session->port, asio::buffer("\n", 1), ec);
        (void)ec;
    } catch(...) {
        // Ignore errors; best-effort drain only
    }

    // Return the session pointer cast to our integer handle type
    return reinterpret_cast<MicrowaveHandle>(session);
}

DLL_EXPORT int32_t close_microwave_controller(MicrowaveHandle handle) {
    if (handle == 0) {
        return API_ERROR_BAD_HANDLE;
    }
    // Cast the handle back to a pointer
    MicrowaveSession* session = reinterpret_cast<MicrowaveSession*>(handle);

    try {
        if (session->port.is_open()) {
            session->port.close();
        }
    } catch (const asio::system_error& e) {
        std::cerr << "Error on port close: " << e.what() << std::endl;
        // Continue to delete, as we can't recover
    }

    delete session; // Free the memory
    return API_SUCCESS;
}

/**
 * @brief Sends a raw command string to the Arduino.
 *
 * NOTE: This function sends the command *verbatim*. The caller is
 * responsible for formatting it correctly (e.g., "press start",
 * "hold 1", "release").
 */
DLL_EXPORT int32_t send_microwave_command(MicrowaveHandle handle, const char* command) {
    if (handle == 0) {
        return API_ERROR_BAD_HANDLE;
    }
    MicrowaveSession* session = reinterpret_cast<MicrowaveSession*>(handle);
    if (!command) {
        return API_ERROR_UNKNOWN;
    }

    // Pass the command string directly to the raw helper
    return send_raw_command(session, std::string(command));
}

DLL_EXPORT int32_t run_microwave(MicrowaveHandle handle, const char* time_str, uint8_t power_level) {
    if (handle == 0) {
        return API_ERROR_BAD_HANDLE;
    }
    MicrowaveSession* session = reinterpret_cast<MicrowaveSession*>(handle);
    if (!time_str) {
        return API_ERROR_BAD_TIME_STR;
    }

    // 1. Parse time
    std::string time_digits;
    if (!parse_time_to_digits(std::string(time_str), time_digits)) {
        return API_ERROR_BAD_TIME_STR;
    }

    // 2. Calculate power presses
    // Logic: 100% (or invalid) -> 0 presses
    //        90% -> (100-90)/10 = 1 press
    //        10% -> (100-10)/10 = 9 presses
    int num_power_presses = 0;
    if (power_level > 100 || power_level < 10 || (power_level % 10 != 0)) {
        num_power_presses = 0; // Default to 100% (0 presses)
    } else if (power_level == 100) {
        num_power_presses = 0;
    } else {
        num_power_presses = (100 - power_level) / 10 + 1;
    }

    // 3. Execute the full command sequence
    int32_t result = API_SUCCESS;

    // Use goto for a clean, single-exit-point C-style API

    //NOTE: does not auto clear assumes clean state

    // Press "Cook Time"
    result = send_raw_command(session, "press cook_time");
    if (result != API_SUCCESS) goto cleanup;

    // Press the digits
    for (char const &digit : time_digits) {
        result = send_raw_command(session, std::string("press ") + digit);
        if (result != API_SUCCESS) goto cleanup;
    }

    // Press "Power" N times
    for (int i = 0; i < num_power_presses; ++i) {
        result = send_raw_command(session, "press power");
        if (result != API_SUCCESS) goto cleanup;
    }

cleanup:
    return result;
}

DLL_EXPORT int32_t stop_microwave(MicrowaveHandle handle) {
    if (handle == 0) {
        return API_ERROR_BAD_HANDLE;
    }
    MicrowaveSession* session = reinterpret_cast<MicrowaveSession*>(handle);

    // This is a simple wrapper for a single raw command
    return send_raw_command(session, "press stop");
}


#ifdef __cplusplus
} // extern "C"
#endif