// glsonyhid.cpp — raw-HID Sony controller effects for the OpenGL backend.
//
// Windows: SetupAPI enumeration + hid.dll (loaded dynamically) to send output
// reports. Linux: enumerate /sys/class/hidraw and write to /dev/hidrawN.
// USB is supported (DualSense report 0x02 / DualShock 4 report 0x05); Bluetooth
// needs a CRC-framed report and is not implemented yet.
#include "pddi/gl/glsonyhid.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#elif defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#endif

namespace {
    // Returns the USB effects report id for a DualSense/DualShock product id
    // and whether it is a DualSense. 0 if unsupported.
    struct SonyModel {
        unsigned int productId = 0;
        bool dualSense = false;
        bool valid = false;
    };

    SonyModel ClassifyProduct(unsigned int pid) {
        SonyModel m;
        m.productId = pid;
        switch (pid) {
            case 0x0CE6:  // DualSense
            case 0x0DF2:  // DualSense Edge
                m.dualSense = true;
                m.valid = true;
                break;
            case 0x05C4:  // DualShock 4 v1
            case 0x09CC:  // DualShock 4 v2
                m.dualSense = false;
                m.valid = true;
                break;
            default:
                break;
        }
        return m;
    }
}

#if defined(_WIN32)

namespace {
    HANDLE sDevice = INVALID_HANDLE_VALUE;
    unsigned int sProductId = 0;

    typedef void (WINAPI* PFN_GetHidGuid)(LPGUID);
    typedef BOOLEAN (WINAPI* PFN_GetAttributes)(HANDLE, PHIDD_ATTRIBUTES);
    typedef BOOLEAN (WINAPI* PFN_SetOutputReport)(HANDLE, PVOID, ULONG);

    PFN_GetHidGuid pGetHidGuid = nullptr;
    PFN_GetAttributes pGetAttributes = nullptr;
    PFN_SetOutputReport pSetOutputReport = nullptr;
}

namespace glsonyhid {

bool EnsureDevice() {
    if (sDevice != INVALID_HANDLE_VALUE) {
        return true;
    }

    HMODULE hid = LoadLibraryA("hid.dll");
    if (!hid) {
        return false;
    }
    pGetHidGuid = (PFN_GetHidGuid)GetProcAddress(hid, "HidD_GetHidGuid");
    pGetAttributes = (PFN_GetAttributes)GetProcAddress(hid, "HidD_GetAttributes");
    pSetOutputReport = (PFN_SetOutputReport)GetProcAddress(hid, "HidD_SetOutputReport");
    if (!pGetHidGuid || !pGetAttributes || !pSetOutputReport) {
        return false;
    }

    GUID guid = {};
    pGetHidGuid(&guid);

    HDEVINFO info = SetupDiGetClassDevsA(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info == INVALID_HANDLE_VALUE) {
        return false;
    }

    SP_DEVICE_INTERFACE_DATA iface = {};
    iface.cbSize = sizeof(iface);
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(info, nullptr, &guid, i, &iface); i++) {
        DWORD needed = 0;
        SetupDiGetDeviceInterfaceDetailA(info, &iface, nullptr, 0, &needed, nullptr);
        if (needed == 0) {
            continue;
        }
        SP_DEVICE_INTERFACE_DETAIL_DATA_A* detail =
            (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(needed);
        if (!detail) {
            break;
        }
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        if (SetupDiGetDeviceInterfaceDetailA(info, &iface, detail, needed, nullptr, nullptr)) {
            HANDLE handle = CreateFileA(detail->DevicePath, GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                        OPEN_EXISTING, 0, nullptr);
            if (handle != INVALID_HANDLE_VALUE) {
                HIDD_ATTRIBUTES attrs = {};
                attrs.Size = sizeof(attrs);
                if (pGetAttributes(handle, &attrs) && attrs.VendorID == 0x054C) {
                    SonyModel model = ClassifyProduct(attrs.ProductID);
                    if (model.valid) {
                        sDevice = handle;
                        sProductId = attrs.ProductID;
                        std::fprintf(stderr, "glsonyhid: device open (product 0x%04X)\n", sProductId);
                        free(detail);
                        SetupDiDestroyDeviceInfoList(info);
                        return true;
                    }
                }
                CloseHandle(handle);
            }
        }
        free(detail);
    }
    SetupDiDestroyDeviceInfoList(info);
    return false;
}

bool Send(unsigned char weak, unsigned char strong,
          unsigned char r, unsigned char g, unsigned char b) {
    if (!EnsureDevice()) {
        return false;
    }
    SonyModel model = ClassifyProduct(sProductId);
    if (!model.valid || !model.dualSense) {
        return false;  // lightbar/DS4 report not implemented
    }

    unsigned char report[48] = {};
    report[0] = 0x02;
    report[1] = 0x03;  // rumble emulation + disable audio haptics
    report[2] = 0x04;  // enable lightbar colour
    report[3] = weak;
    report[4] = strong;
    report[45] = r;
    report[46] = g;
    report[47] = b;
    return pSetOutputReport(sDevice, report, sizeof(report)) != FALSE;
}

void Release() {
    if (sDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(sDevice);
        sDevice = INVALID_HANDLE_VALUE;
    }
}

}

#elif defined(__linux__)

namespace {
    int sFd = -1;
    unsigned int sProductId = 0;

    bool UeventMatches(const std::string& path, unsigned int* outProduct) {
        FILE* f = fopen(path.c_str(), "r");
        if (!f) {
            return false;
        }
        char line[256];
        bool found = false;
        while (fgets(line, sizeof(line), f)) {
            unsigned int bus = 0, vid = 0, pid = 0;
            if (sscanf(line, "HID_ID=%x:%x:%x", &bus, &vid, &pid) == 3) {
                if (vid == 0x054C) {
                    SonyModel model = ClassifyProduct(pid);
                    if (model.valid) {
                        *outProduct = pid;
                        found = true;
                    }
                }
            }
        }
        fclose(f);
        return found;
    }
}

namespace glsonyhid {

bool EnsureDevice() {
    if (sFd >= 0) {
        return true;
    }
    DIR* dir = opendir("/sys/class/hidraw");
    if (!dir) {
        return false;
    }
    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "hidraw", 6) != 0) {
            continue;
        }
        std::string uevent = std::string("/sys/class/hidraw/") + entry->d_name + "/device/uevent";
        unsigned int product = 0;
        if (!UeventMatches(uevent, &product)) {
            continue;
        }
        std::string devPath = std::string("/dev/") + entry->d_name;
        int fd = open(devPath.c_str(), O_RDWR | O_NONBLOCK);
        if (fd >= 0) {
            sFd = fd;
            sProductId = product;
            std::fprintf(stderr, "glsonyhid: device open %s (product 0x%04X)\n",
                         devPath.c_str(), sProductId);
            break;
        }
    }
    closedir(dir);
    return sFd >= 0;
}

bool Send(unsigned char weak, unsigned char strong,
          unsigned char r, unsigned char g, unsigned char b) {
    if (!EnsureDevice()) {
        return false;
    }
    SonyModel model = ClassifyProduct(sProductId);
    if (!model.valid || !model.dualSense) {
        return false;
    }

    unsigned char report[48] = {};
    report[0] = 0x02;
    report[1] = 0x03;
    report[2] = 0x04;
    report[3] = weak;
    report[4] = strong;
    report[45] = r;
    report[46] = g;
    report[47] = b;
    const ssize_t written = write(sFd, report, sizeof(report));
    return written == (ssize_t)sizeof(report);
}

void Release() {
    if (sFd >= 0) {
        close(sFd);
        sFd = -1;
    }
}

}

#else

namespace glsonyhid {
    bool EnsureDevice() { return false; }
    bool Send(unsigned char, unsigned char, unsigned char, unsigned char, unsigned char) { return false; }
    void Release() {}
}

#endif
