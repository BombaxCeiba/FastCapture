#include "FastCaptureInjectDll.h"
#include <string>
#include <Windows.h>
#include "GlReadPixelsThread.h"
#include "DllData.h"
#include "../FastCaptureInjectDllDef.h"
#include "../../Utils/Windows/UtilsWindows.hpp"
#include "GL/gl.h"

DWORD WINAPI FastCaptureInitInjectDll(LPVOID lpThreadParameter) FAST_CAPTURE_NOEXCEPT
{
    if (lpThreadParameter == nullptr)
    {
        return static_cast<DWORD>(FAST_CAPTURE_E_INVALID_ARGUMENT);
    }
    auto& dll_data = FAST_CAPTURE::DllData::GetInstance();
    auto p_w_shared_memory_name_prefix = static_cast<wchar_t*>(lpThreadParameter);
    dll_data.shared_memory_name_prefix_ = p_w_shared_memory_name_prefix;
    auto capture_descriptor_shared_name =
        std::wstring(L"Global\\") + dll_data.shared_memory_name_prefix_ + std::wstring(L"Descriptor");
    FAST_CAPTURE::Windows::UniqueHandleInvalidNULL h_capture_descriptor = ::CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(FAST_CAPTURE::CaptureDescriptor),
        capture_descriptor_shared_name.c_str());
    if (h_capture_descriptor.IsInvalid())
    {
        return FAST_CAPTURE_E_CREATE_SHARED_CAPTURE_DESCRIPTOR_FAILED;
    }
    FAST_CAPTURE::Windows::UniqueMapViewOfFile<FAST_CAPTURE::CaptureDescriptor> p_shared_capture_descriptor =
        reinterpret_cast<FAST_CAPTURE::CaptureDescriptor*>(
            ::MapViewOfFile(
                h_capture_descriptor.Get(),
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                sizeof(FAST_CAPTURE::CaptureDescriptor)));
    if (p_shared_capture_descriptor.IsInvalid())
    {
        return FAST_CAPTURE_E_CREATE_SHARED_CAPTURE_DESCRIPTOR_MAP_OF_VIEW_FAILED;
    }
    dll_data.p_capture_descriptor_ = std::move(p_shared_capture_descriptor);
    dll_data.h_capture_descriptor_ = std::move(h_capture_descriptor);
    std::ignore = FAST_CAPTURE::ReadPixelsThread::GetInstance();
    return FAST_CAPTURE_S_OK;
}

DWORD WINAPI FastCaptureDestroyDll([[maybe_unused]] LPVOID lpThreadParameter) FAST_CAPTURE_NOEXCEPT
{
    FAST_CAPTURE::ReadPixelsThread::GetInstance().RequestThreadStop();
    return FAST_CAPTURE_S_OK;
}

FAST_CAPTURE_NAMESPACE
{
    // BOOL WINAPI wglSwapLayerBuffersFake(HDC unnamedParam1, UINT unnamedParam2) {}
}

BOOL WINAPI DllMain(
    [[maybe_unused]] HINSTANCE hinstDLL, // handle to DLL module
    [[maybe_unused]] DWORD fdwReason,    // reason for calling function
    [[maybe_unused]] LPVOID lpvReserved) // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.
        break;

    case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;

    case DLL_PROCESS_DETACH:

        if (lpvReserved != nullptr)
        {
            break; // do not do cleanup if process termination scenario
        }

        // Perform any necessary cleanup.
        break;
    }
    return TRUE; // Successful DLL_PROCESS_ATTACH.
}
