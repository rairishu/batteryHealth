#define INITGUID
//https://learn.microsoft.com/en-us/windows/win32/power/enumerating-battery-devices
#include <windows.h>
#include <iostream>
#include <batclass.h> // Include the necessary header for battery structures
#include<setupapi.h>
#include <devguid.h>

using namespace std;

typedef struct _BATTERY_WMI_CYCLE_COUNT {
    ULONG Tag;
    ULONG CycleCount;
} BATTERY_WMI_CYCLE_COUNT, * PBATTERY_WMI_CYCLE_COUNT;


DWORD GetBatteryState()
{
#define GBS_HASBATTERY 0x1
#define GBS_ONBATTERY  0x2
    // Returned value includes GBS_HASBATTERY if the system has a 
    // non-UPS battery, and GBS_ONBATTERY if the system is running on 
    // a battery.
    //
    // dwResult & GBS_ONBATTERY means we have not yet found AC power.
    // dwResult & GBS_HASBATTERY means we have found a non-UPS battery.

    DWORD dwResult = GBS_ONBATTERY;

    // IOCTL_BATTERY_QUERY_INFORMATION,
    // enumerate the batteries and ask each one for information.

    HDEVINFO hdev =
        SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY,
            0,
            0,
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (INVALID_HANDLE_VALUE != hdev)
    {
        // Limit search to 100 batteries max
        for (int idev = 0; idev < 100; idev++)
        {
            SP_DEVICE_INTERFACE_DATA did = { 0 };
            did.cbSize = sizeof(did);

            if (SetupDiEnumDeviceInterfaces(hdev,
                0,
                &GUID_DEVCLASS_BATTERY,
                idev,
                &did))
            {
                DWORD cbRequired = 0;

                SetupDiGetDeviceInterfaceDetail(hdev,
                    &did,
                    0,
                    0,
                    &cbRequired,
                    0);
                if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
                {
                    PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd =
                        (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR,
                            cbRequired);
                    if (pdidd)
                    {
                        pdidd->cbSize = sizeof(*pdidd);
                        if (SetupDiGetDeviceInterfaceDetail(hdev,
                            &did,
                            pdidd,
                            cbRequired,
                            &cbRequired,
                            0))
                        {
                            // Enumerated a battery.  Ask it for information.
                            HANDLE hBattery =
                                CreateFile(pdidd->DevicePath,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
                            if (INVALID_HANDLE_VALUE != hBattery)
                            {
                                // Ask the battery for its tag.
                                BATTERY_QUERY_INFORMATION bqi = { 0 };

                                DWORD dwWait = 0;
                                DWORD dwOut;

                                if (DeviceIoControl(hBattery,
                                    IOCTL_BATTERY_QUERY_TAG,
                                    &dwWait,
                                    sizeof(dwWait),
                                    &bqi.BatteryTag,
                                    sizeof(bqi.BatteryTag),
                                    &dwOut,
                                    NULL)
                                    && bqi.BatteryTag)
                                {
                                    // With the tag, you can query the battery info.
                                    BATTERY_INFORMATION bi = { 0 };
                                    bqi.InformationLevel = BatteryInformation;

                                    if (DeviceIoControl(hBattery,
                                        IOCTL_BATTERY_QUERY_INFORMATION,
                                        &bqi,
                                        sizeof(bqi),
                                        &bi,
                                        sizeof(bi),
                                        &dwOut,
                                        NULL))
                                    {
                                        cout << bi.CycleCount << " ";
                                        // Only non-UPS system batteries count
                                        if (bi.Capabilities & BATTERY_SYSTEM_BATTERY)
                                        {
                                            if (!(bi.Capabilities & BATTERY_IS_SHORT_TERM))
                                            {
                                                dwResult |= GBS_HASBATTERY;
                                            }

                                            // Query the battery status.
                                            BATTERY_WAIT_STATUS bws = { 0 };
                                            bws.BatteryTag = bqi.BatteryTag;

                                            BATTERY_STATUS bs;
                                            if (DeviceIoControl(hBattery,
                                                IOCTL_BATTERY_QUERY_STATUS,
                                                &bws,
                                                sizeof(bws),
                                                &bs,
                                                sizeof(bs),
                                                &dwOut,
                                                NULL))
                                            {
                                                if (bs.PowerState & BATTERY_POWER_ON_LINE)
                                                {
                                                    dwResult &= ~GBS_ONBATTERY;
                                                }
                                            }
                                        }
                                    }
                                }
                                CloseHandle(hBattery);
                            }
                        }
                        LocalFree(pdidd);
                    }
                }
            }
            else  if (ERROR_NO_MORE_ITEMS == GetLastError())
            {
                break;  // Enumeration failed - perhaps we're out of items
            }
        }
        SetupDiDestroyDeviceInfoList(hdev);
    }

    //  Final cleanup:  If we didn't find a battery, then presume that we
    //  are on AC power.

    if (!(dwResult & GBS_HASBATTERY))
        dwResult &= ~GBS_ONBATTERY;

    return dwResult;
}
int main() {
    HANDLE hBattery = CreateFile(L"\\\\.\\Battery1", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    GetBatteryState();
    if (hBattery == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open battery device." << std::endl;
        return 1;
    }
    

    BATTERY_QUERY_INFORMATION batteryQueryInfo;
    BATTERY_INFORMATION batteryInfo;
    DWORD bytesReturned;

    batteryQueryInfo.BatteryTag = 0;
    batteryQueryInfo.InformationLevel = BatteryInformation;
    batteryQueryInfo.AtRate = 0;

    BOOL result = DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION, &batteryQueryInfo, sizeof(batteryQueryInfo), &batteryInfo, sizeof(batteryInfo), &bytesReturned, NULL);
    if (result) {
        // std::cout << "Battery Cycle Count: " << batteryInfo.CycleCount << std::endl;
        int x;
        std::cin >> x;
    }
    else {
        std::cerr << "DeviceIoControl failed." << std::endl;
    }

    CloseHandle(hBattery);
    return 0;
}
