# GpuModeDriver Documentation

## Overview
The `GpuModeDriver` is a kernel-mode driver built using the Windows Driver Framework (WDF) for Windows 11. It enables switching between integrated and discrete GPUs on MSI laptops by leveraging ACPI methods specific to MSI systems, such as `HPEM`, `_SB.SGII`, and `_PS0`/`_PS3`. The driver exposes an IOCTL interface for user-mode applications to set or retrieve the current GPU mode.

**Key Features:**
- Switch between integrated (0) and discrete (1) GPU modes.
- Retrieve the current GPU mode.
- Designed for MSI laptops with hybrid GPU setups.
- Requires administrative privileges to interact with the driver.

**Note:** This driver is tailored for MSI laptops and may not work on other systems without modification.

---

## Build Instructions
To build the `GpuModeDriver`, you will need:
- **Visual Studio 2022** with the **Windows Driver Kit (WDK)** installed.
- A test certificate for signing the driver (for development purposes).

**Steps to Build:**
1. Open **Visual Studio 2022**.
2. Select **Create a new project**.
3. Choose **Kernel Mode Driver (KMDF)** from the template list.
4. Name the project `GpuModeDriver` and click **Create**.
5. Replace the default source files with `GpuModeDriver.c` and `GpuModeDriver.h` (provided).
6. Ensure the project is set to **Debug** and **x64** in the configuration dropdown.
7. Build the solution by pressing **Ctrl+Shift+B** or selecting **Build > Build Solution**.
8. The driver binary (`GpuModeDriver.sys`) will be generated in the `x64\Debug` folder.

**Signing the Driver (Development):**
- Create a test certificate using `makecert`:
  ```
  makecert -r -pe -ss PrivateCertStore -n "CN=TestCert" TestCert.cer
  ```
- Sign the driver with `signtool`:
  ```
  signtool sign /s PrivateCertStore /n TestCert /t http://timestamp.digicert.com GpuModeDriver.sys
  ```
- Enable test signing on your machine:
  ```
  bcdedit /set testsigning on
  ```
- Reboot your machine.

---

## Installation
To install the driver on your MSI laptop:
1. **Enable Test Signing**:
   - Open an elevated Command Prompt.
   - Run: `bcdedit /set testsigning on`.
   - Reboot your machine.
2. **Copy Driver Files**:
   - Copy `GpuModeDriver.sys`, `GpuModeDriver.inf`, and `GpuModeDriver.cat` (if signed) to a directory on the target machine (e.g., `C:\Driver`).
3. **Install the Driver**:
   - Right-click `GpuModeDriver.inf` and select **Install**.
   - Alternatively, use `devcon`:
     ```
     devcon install GpuModeDriver.inf ROOT\SYSTEM\0000
     ```
4. **Verify Installation**:
   - Open **Device Manager**.
   - Look for "GpuModeDriver Device" under **System devices**.

---

## Usage Guide
The driver provides two IOCTLs for interacting with the GPU modes:
- **`IOCTL_SET_GPU_MODE`**: Sets the GPU mode.
  - **Input**: A `ULONG` value (0 for integrated, 1 for discrete).
  - **Output**: None.
- **`IOCTL_GET_GPU_MODE`**: Retrieves the current GPU mode.
  - **Input**: None.
  - **Output**: A `ULONG` value (0 for integrated, 1 for discrete).

**Interacting with the Driver:**
- Create a user-mode application (e.g., in C++ or C#) to open the device (`\\.\GpuModeDriver`) and send IOCTLs using `DeviceIoControl`.
- **Example (C++)**:
  ```cpp
  #include <windows.h>
  #include <winioctl.h>

  #define IOCTL_SET_GPU_MODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_WRITE_ACCESS)
  #define IOCTL_GET_GPU_MODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)

  int main() {
      HANDLE hDevice = CreateFile(L"\\\\.\\GpuModeDriver", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
      if (hDevice == INVALID_HANDLE_VALUE) {
          // Handle error
          return 1;
      }

      // Set GPU mode to discrete (1)
      ULONG mode = 1;
      DWORD bytesReturned;
      DeviceIoControl(hDevice, IOCTL_SET_GPU_MODE, &mode, sizeof(mode), NULL, 0, &bytesReturned, NULL);

      // Get current GPU mode
      ULONG currentMode;
      DeviceIoControl(hDevice, IOCTL_GET_GPU_MODE, NULL, 0, &currentMode, sizeof(currentMode), &bytesReturned, NULL);
      // Use currentMode as needed

      CloseHandle(hDevice);
      return 0;
  }
  ```

**Important**: After setting the GPU mode, a system restart may be required for the changes to take effect, as GPU mode switching often involves firmware-level reconfiguration.

---

## Troubleshooting
- **Build Errors**:
  - Ensure the WDK is correctly installed and configured in Visual Studio.
  - Check for syntax errors or missing includes in the source files.
- **Installation Failures**:
  - Verify that test signing is enabled (`bcdedit /set testsigning on`).
  - Ensure the INF file is correctly formatted and references the driver binary.
- **IOCTL Issues**:
  - Confirm the driver is installed and running (`sc query GpuModeDriver`).
  - Use DebugView to capture `KdPrint` logs for debugging.
- **ACPI Method Failures**:
  - Ensure the ACPI methods All(`HPEM`, `_SB.SGII`, etc.) match those in your MSI laptop’s DSDT.
  - Use tools like RW-Everything or acpi_call (on Linux) to verify method names and arguments.

---

## Additional Resources
- [Windows Driver Documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/)
- [ACPI Specification](https://uefi.org/specifications)
- [MSI DSDT Analysis](https://www.tonymacx86.com/forums/acpi.14/) (for reference)

This documentation provides a foundation for building, installing, and using the `GpuModeDriver`. For further assistance, refer to the additional resources or contact the developer.