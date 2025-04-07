# Lua Notifier Kernel Module

A Linux kernel module that provides a character device interface for capturing and storing messages. This module can be used to monitor and log kernel messages through a user-space interface.

## Features

- Character device interface at `/dev/luanotifier`
- Thread-safe message storage
- Support for both read and write operations
- Debug logging through kernel messages
- Configurable buffer size for messages

## Requirements

- Linux kernel headers
- Build tools (make, gcc)
- Root/sudo access for module loading

## Installation

1. Clone the repository:
```bash
git clone <repository-url>
cd lua
```

2. Build the module:
```bash
make -f kbuild.mk clean
make -f kbuild.mk
```

3. Load the module:
```bash
sudo insmod luanotifier.ko
```

## Usage

### Writing Messages
```bash
# Write a message to the device
echo "Your message" | sudo tee /dev/luanotifier
```

### Reading Messages
```bash
# Read all stored messages
sudo cat /dev/luanotifier
```

### Monitoring Kernel Messages
```bash
# View module debug messages
sudo dmesg | grep "Lua Notifier"
```

## Module Details

### Device Information
- Device Name: `luanotifier`
- Major Number: Dynamically allocated
- Minor Number: 0
- Device Path: `/dev/luanotifier`

### Configuration
- Maximum Events: 100
- Maximum Event Size: 256 bytes

### Build System
- Uses kernel build system
- Supports verbose output for debugging
- Includes clean target for removing build artifacts

## Development

### Building
```bash
# Clean previous builds
make -f kbuild.mk clean

# Build the module
make -f kbuild.mk
```

### Loading/Unloading
```bash
# Load the module
sudo insmod luanotifier.ko

# Unload the module
sudo rmmod luanotifier
```

### Debugging
```bash
# View kernel messages
sudo dmesg | tail

# Check module status
lsmod | grep luanotifier
```

## License

This project is licensed under the GPL License - see the LICENSE file for details.

## Author

Shubhangi

## Acknowledgments

- Linux Kernel Documentation
- Linux Device Drivers, 3rd Edition
