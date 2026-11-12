#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

#if defined(__APPLE__) || defined(__linux__)
#include <glob.h>
#endif

#ifdef _WIN32
using SerialHandle = HANDLE;
#else
using SerialHandle = int;
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

  throw std::runtime_error("Please specify the Windows COM port using "
                           "SEL_SiPM_PORT, for example COM5.");

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

void configure_serial_port(SerialHandle serial_handle) {
#ifdef _WIN32
  DCB settings{}; // Windows specific structure for the port-parameters
  settings.DCBlength = sizeof(settings);

  if (!GetCommState(serial_handle, &settings)) {
    throw std::runtime_error("Failed to read serial settings (Windows error " +
                             std::to_string(GetLastError()) + ").");
  }

  // configure the UART settings the FPGA expects
  settings.BaudRate = CBR_115200;
  settings.ByteSize = 8;
  settings.Parity = NOPARITY;
  settings.StopBits = ONESTOPBIT;
  settings.fBinary = TRUE;
  settings.fParity = FALSE;
  settings.fOutxCtsFlow = FALSE;
  settings.fRtsControl = RTS_CONTROL_DISABLE;

  // configure the serial port to the settings that have been configure above
  if (!SetCommState(serial_handle, &settings)) {
    throw std::runtime_error("Failed to configure serial port (Windows error " +
                             std::to_string(GetLastError()) + ").");
  }

  // sanity-check if everything is configured correctly
  COMMTIMEOUTS timeouts{};
  timeouts.ReadIntervalTimeout = MAXDWORD;
  if (!SetCommTimeouts(serial_handle, &timeouts)) {
    throw std::runtime_error(
        "Failed to configure serial timeouts (Windows error " +
        std::to_string(GetLastError()) + ").");
  }
#else
  const int file_descriptor = serial_handle;
  termios settings{};

  // read the current configuration of the serial port and safe it in settings
  if (tcgetattr(file_descriptor, &settings) != 0) {
    throw std::runtime_error(std::string("Failed to read serial settings: ") +
                             std::strerror(errno));
  }

  // make the serial port "raw" meaning every byte is transmitted unchanged
  cfmakeraw(&settings);

  // configure the UART Baudrate
  if (cfsetispeed(&settings, B115200) != 0 ||
      cfsetospeed(&settings, B115200) != 0) {
    throw std::runtime_error("Failed to select 115200 baud.");
  }

  settings.c_cflag &= ~PARENB; // no parity bit
  settings.c_cflag &= ~CSTOPB; // enable the transmission of a stop-bit
  settings.c_cflag &= ~CSIZE;  // remove the old bit-size
  settings.c_cflag |=
      CS8 | CLOCAL |
      CREAD; // set the new bit-size to 8 databits, ignore the modem-control
             // since the FPGA needs to be connected for communication, allow to
             // receive data
  settings.c_cflag &=
      ~CRTSCTS; // data is directly transmitted the moment it is send

  // set the new attributs for the serial port now
  if (tcsetattr(file_descriptor, TCSANOW, &settings) != 0) {
    throw std::runtime_error(std::string("Failed to configure serial port: ") +
                             std::strerror(errno));
  }
#endif
}

void send_command(SerialHandle serial_handle, char command) {
#ifdef _WIN32
  DWORD bytes_written = 0;
  if (!WriteFile(serial_handle, &command, 1, &bytes_written, nullptr) ||
      bytes_written != 1) {
    throw std::runtime_error("Failed to send UART command (Windows error " +
                             std::to_string(GetLastError()) + ").");
  }
#else
  const int file_descriptor = serial_handle;
  const ssize_t bytes_written = write(file_descriptor, &command, 1);

  if (bytes_written != 1) {
    throw std::runtime_error(std::string("Failed to send UART command: ") +
                             std::strerror(errno));
  }
#endif
}

void wait_for_acknowledgement(SerialHandle serial_handle, char command) {
  // this is the format the FPGA is configured to send
  const std::string expected = "ACK " + std::string(1, command) + "\n";

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(100);

  for (const char expected_byte : expected) {
    const auto remaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());

    if (remaining.count() <= 0) {
      throw std::runtime_error("Timed out waiting for FPGA acknowledgement.");
    }

    char received_byte = '\0';

#ifdef _WIN32
    bool byte_available = false;
    // loop while through the if statements while waiting for a byte, if an
    // error occurs or the acknowledgment takes too long, the program throws an
    // runtime error
    while (!byte_available) {
      COMSTAT status{};
      DWORD errors = 0;
      if (!ClearCommError(serial_handle, &errors, &status)) {
        throw std::runtime_error(
            "Failed while waiting for acknowledgement (Windows error " +
            std::to_string(GetLastError()) + ").");
      }
      byte_available = status.cbInQue > 0;
      if (!byte_available) {
        // wait for 1ms to have no "constant" reading which would unnecessary
        // load the CPU
        Sleep(1);
        if (std::chrono::steady_clock::now() >= deadline) {
          throw std::runtime_error(
              "Timed out waiting for FPGA acknowledgement.");
        }
      }
    }

    // read the byte and store it in received_byte, check if exactly one byte
    // has been read and no error occured
    DWORD bytes_read = 0;
    if (!ReadFile(serial_handle, &received_byte, 1, &bytes_read, nullptr) ||
        bytes_read != 1) {
      throw std::runtime_error(
          "Failed to read FPGA acknowledgement (Windows error " +
          std::to_string(GetLastError()) + ").");
    }
#else
    const int file_descriptor = serial_handle;
    pollfd descriptor{
        file_descriptor, POLLIN,
        0}; // construct to read data from the configured serial port, POLLIN
    const int poll_result = poll(
        &descriptor, 1,
        static_cast<int>(remaining.count())); // wait until one byte has been
                                              // received or the counter ran out

    if (poll_result == 0) {
      throw std::runtime_error("Timed out waiting for FPGA acknowledgement.");
    }
    if (poll_result < 0) {
      throw std::runtime_error(
          std::string("Failed while waiting for acknowledgement: ") +
          std::strerror(errno));
    }

    if (read(file_descriptor, &received_byte, 1) != 1) {
      throw std::runtime_error(
          std::string("Failed to read FPGA acknowledgement: ") +
          std::strerror(errno));
    }
#endif

    // check if the received byte equals the expected byte, which is known by
    // construction
    if (received_byte != expected_byte) {
      throw std::runtime_error("Unexpected FPGA acknowledgement.");
    }
  }
}

// ------------------------------------------------------------
// Main program
// ------------------------------------------------------------

int main(int argc, char *argv[]) {

  // Exactly one command is expected
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0]
              << " <LED_1|LED_2|LED_3|start_pulsing|stop_pulsing|reset>\n";

    return 1;
  }

  const std::string command_name = argv[1];
  char command = '\0';

  // Map the user commands to the corresponding commands the FPGA expects
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
    SerialHandle serial_handle;

#ifdef _WIN32
    const std::string device_name =
        "\\\\.\\" + port_name; // portname in Windows: \\.\COM{X}
    serial_handle = CreateFileA(
        device_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, 0,
        nullptr); // open the port exclusively for reading and writing, sanity
                  // check if the port is even available (OPEN_EXISTING)
    if (serial_handle == INVALID_HANDLE_VALUE) {
      throw std::runtime_error("Failed to open serial port (Windows error " +
                               std::to_string(GetLastError()) + ").");
    }
#else
    serial_handle =
        open(port_name.c_str(),
             O_RDWR | O_NOCTTY); // open the port for reading and writing
    if (serial_handle < 0) {
      throw std::runtime_error(std::string("Failed to open serial port: ") +
                               std::strerror(errno));
    }
#endif

    // configure UART
    configure_serial_port(serial_handle);

    // send exactly the one byte command
    send_command(serial_handle, command);

    // ckeck if the command has been received
    wait_for_acknowledgement(serial_handle, command);

    std::cout << "Command '" << command_name << "' acknowledged by the FPGA.\n";

// close the opened ports
#ifdef _WIN32
    CloseHandle(serial_handle);
#else
    close(serial_handle);
#endif
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
