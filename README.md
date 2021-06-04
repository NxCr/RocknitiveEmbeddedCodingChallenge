# Rocknitive Embedded Coding Challenge

The aim of this coding challenge is to prove general development skills and basic understanding of the typical challenges in embedded development.

## Task

In this challenge you are required to handle communication over a (simulated) serial interface. Your program must have the following features:

- Receive data via a (simulated) interrupt.
- Parse incoming data for messages defined in the protocol below.
- Send the required responses.
- Print all messages (received and sent) in a human readable format to the console.

Upload your solution as a git repository to one of the public platforms (e.g. GitHub or Bitbucket).

## Protocol

- Messages are encoded as binary TLV (type-length-value) messages.
- The type and length field are encoded as the higher and lower four bits, respectively, of the first byte of each message.
- The value contains the payload with as many bytes as given by the length value.

### Message types

| Type | Length | Name    | Payload                                      |
|------|--------|---------|----------------------------------------------|
| 1    | 0      | Empty   | None                                         |
| 2    | 4      | Add     | Two 16-bit unsigned integers                 |
| 3    | 2      | Result  | One 16-bit unsigned integer                  |
| 4    | 3      | Delay   | 16-bit unsigned integer and opaque data byte |
| 5    | 1      | Timeout | Opaque data byte                             |
| 6    | N      | Log     | String of N ASCII encoded characters         |

### Responses

- Respond to `Empty` messages with an `Empty` message.
- Respond to `Add` message with a `Result` message containing the sum of the numbers in the `Add` message.
- Respond to `Delay` messages with a `Timeout` message. The integer in the `Delay` message represents a duration in milliseconds. The `Timeout` message should be send after this duration has elapsed. The payload of the `Timeout` message should contain the data byte received with the `Delay` message.

## Additional information

- Add your code in `src/challenge.c`, none of the other source file should be modified.
- The project is built using CMake and GCC. If you add additional source files they need to be added to the CMakeLists.txt file.
- All data must be encoded in network byte-order, i.e. big-endian.
- This challenge uses FreeRTOS (Posix-Port) to simulate hardware interrupts and preemption on a desktop machine.
- You can also use FreeRTOS features to implement your solution. This is recommended, but not mandatory.
- The test cases don't contain multiple pending delays simultaneously, but you get bonus points if your solution could handle that.

## Installation / Dependencies

- Install dependencies:
    - Windows: Use the Windows-Subsytem for Linux (WSL) and do the same as on Ubuntu.
    - Ubuntu: `sudo apt-get install git cmake`
    - MacOS: `brew install git cmake`
- Create a build directory `mkdir build && cd build`
- Setup build: `cmake ..`
- Build: `make`
- Run: `./coding_challenge`
