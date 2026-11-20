#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#ifdef __APPLE__
#include <glob.h>
#endif

#ifdef __linux__
#include <glob.h>
#endif

// ------------------------------------------------------------
// Find the serial port of the Nexys A7
// ------------------------------------------------------------

std::string find_serial_port() {
  // allow to specify the port manually
  // Example MacOS:
  // SEL_SiPM_PORT=/dev/cu.usbserial-XXXX ./sel_SiPM_led LED_1

  const char *env_port = std::getenv("SEL_SiPM_PORT");

  if (env_port != nullptr) {
    return env_port;
  }

#if defined(__APPLE__)

  std::vector<std::string> patterns = {"/dev/cu.usbserial*",
                                       "/dev/cu.usbmodem*"};

#elif defined(__linux__)

  std::vector<std::string> patterns = {"/dev/ttyUSB*", "/dev/ttyACM*"};

#elif defined(_WIN32)

  throw std::runtime_error(
      "Please specify the Windows COM port using SEL_SIPM_PORT.");

#else

  throw std::runtime_error("Unsupported operating system.");

#endif

#if defined(__APPLE__) || defined(__linux__)

  std::vector<std::string> ports;

  for (const auto &pattern : patterns) {
    glob_t result{};

    if (glob(pattern.c_str(), 0, nullptr, &result) == 0) {
      for (std::size_t i = 0; i < result.gl_pathc; ++i) {
        ports.emplace_back(result.gl_pathv[i]);
      }
    }

    globfree(&result);
  }

  if (ports.empty()) {
    throw std::runtime_error(
        "No USB serial port found. Is the FPGA connected???");
  }

  if (ports.size() > 1) {
    std::cerr << "Multiple USB serial ports found:\n";

    for (const auto &port : ports) {
      std::cerr << "  " << port << '\n';
    }

    throw std::runtime_error(
        "Please specify the correct port using SEL_SIPM_PORT.");
  }

  return ports.front();

#endif
}

// ------------------------------------------------------------
// Configure the serial port
// UART: 115200 baud, 8 data bits, no parity, 1 start bit, 1 stop bit
// ------------------------------------------------------------

void configure_serial_port(int file_descriptor) {
  termios settings{};

  if (tcgetattr(file_descriptor, &settings) != 0) {
    throw std::runtime_error(std::string("Failed to read serial settings: ") +
                             std::strerror(errno));
  }

  cfmakeraw(&settings);
  if (cfsetispeed(&settings, B115200) != 0 ||
      cfsetospeed(&settings, B115200) != 0) {
    throw std::runtime_error("Failed to select 115200 baud.");
  }

  settings.c_cflag &= ~PARENB;
  settings.c_cflag &= ~CSTOPB;
  settings.c_cflag &= ~CSIZE;
  settings.c_cflag |= CS8 | CLOCAL | CREAD;
  settings.c_cflag &= ~CRTSCTS;

  if (tcsetattr(file_descriptor, TCSANOW, &settings) != 0) {
    throw std::runtime_error(std::string("Failed to configure serial port: ") +
                             std::strerror(errno));
  }
}

void send_command(int file_descriptor, char command) {
  const ssize_t bytes_written = write(file_descriptor, &command, 1);

  if (bytes_written != 1) {
    throw std::runtime_error(std::string("Failed to send UART command: ") +
                             std::strerror(errno));
  }
}

void wait_for_acknowledgement(int file_descriptor, char command) {
  const std::string expected = "ACK " + std::string(1, command) + "\n";

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);

  for (const char expected_byte : expected) {
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());

    if (remaining.count() <= 0) {
      throw std::runtime_error("Timed out waiting for FPGA acknowledgement.");
    }

    pollfd descriptor{file_descriptor, POLLIN, 0};
    const int poll_result = poll(&descriptor, 1, static_cast<int>(remaining.count()));

    if (poll_result == 0) {
      throw std::runtime_error("Timed out waiting for FPGA acknowledgement.");
    }
    if (poll_result < 0) {
      throw std::runtime_error(std::string("Failed while waiting for acknowledgement: ") +
                               std::strerror(errno));
    }

    char received_byte = '\0';
    if (read(file_descriptor, &received_byte, 1) != 1) {
      throw std::runtime_error(std::string("Failed to read FPGA acknowledgement: ") +
                               std::strerror(errno));
    }
    if (received_byte != expected_byte) {
      throw std::runtime_error("Unexpected FPGA acknowledgement.");
    }
  }
}

// ------------------------------------------------------------
// Main program
// ------------------------------------------------------------

int main(int argc, char *argv[]) {
  // Exactly one command is expected.

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0]
              << " <LED_1|LED_2|LED_3|start_pulsing|stop_pulsing|reset>\n";

    return 1;
  }

  const std::string command_name = argv[1];
  char command = '\0';

  // Map user-facing commands to the one-byte FPGA UART protocol.
  if (command_name == "LED_1" || command_name == "LED_2" ||
      command_name == "LED_3") {
    command = command_name.back();
  } else if (command_name == "start_pulsing") {
    command = 's';
  } else if (command_name == "stop_pulsing") {
    command = 't';
  } else if (command_name == "reset") {
    command = 'r';
  } else {
    std::cerr << "Invalid command: " << command_name << '\n'
              << "Allowed commands are LED_1, LED_2, LED_3, start_pulsing, "
                 "stop_pulsing and reset.\n";
    return 1;
  }

  try {
    // determine the serial port

    const std::string port_name = find_serial_port();

    std::cout << "Using port: " << port_name << '\n';

    // open the serial connection

    const int file_descriptor = open(port_name.c_str(), O_RDWR | O_NOCTTY);
    if (file_descriptor < 0) {
      throw std::runtime_error(std::string("Failed to open serial port: ") +
                               std::strerror(errno));
    }

    // configure UART

    configure_serial_port(file_descriptor);

    // Send exactly one ASCII command character.

    send_command(file_descriptor, command);
    wait_for_acknowledgement(file_descriptor, command);

    std::cout << "Command '" << command_name
              << "' acknowledged by the FPGA.\n";

    close(file_descriptor);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
