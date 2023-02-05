#ifndef FAST_CAPTURE_UTILS_WINDOWS_UTILS_WINDOWS_HPP
#define FAST_CAPTURE_UTILS_WINDOWS_UTILS_WINDOWS_HPP

#include "FastCaptureDef.h"
#include <exception>
#include <Windows.h>
#include <tlhelp32.h>
#include "../Utils.hpp"

#define FAST_CAPTURE_WIN32_ERROR_ARGS FAST_CAPTURE_ERROR_TYPE_WIN32, ::GetLastError()

FAST_CAPTURE_NAMESPACE
{
    namespace Windows
    {
        inline FastCaptureErrorCode MakeError(std::uint16_t fast_capture_error_code)
        {
            return {
                fast_capture_error_code,
                FAST_CAPTURE_WIN32_ERROR_ARGS};
        }

        struct HandleInvalidMinusOneDeleter
        {
            void operator()(HANDLE handle) const noexcept
            {
                if (handle != INVALID_HANDLE_VALUE)
                {
                    ::CloseHandle(handle);
                }
            }
        };
        struct HandleInvalidMinusOneVerifier
        {
            bool operator()(HANDLE handle) const noexcept
            {
                if (handle != INVALID_HANDLE_VALUE && handle != 0)
                {
                    return true;
                }
                return false;
            }
        };
        using UniqueHandleInvalidMinusOne =
            FAST_CAPTURE::Utils::RAIIWrapper<
                HANDLE,
                HandleInvalidMinusOneDeleter,
                HandleInvalidMinusOneVerifier>;

        struct HandleInvalidNULLDeleter
        {
            void operator()(HANDLE handle) const noexcept
            {
                if (handle)
                {
                    ::CloseHandle(handle);
                }
            }
        };
        using UniqueHandleInvalidNULL =
            FAST_CAPTURE::Utils::RAIIWrapper<
                HANDLE,
                HandleInvalidNULLDeleter>;

        struct MapViewOfFileDeleter
        {
            void operator()(void* pointer) const noexcept
            {
                if (pointer)
                {
                    ::UnmapViewOfFile(pointer);
                }
            }
        };
        template <class T>
        using UniqueMapViewOfFile =
            FAST_CAPTURE::Utils::RAIIWrapper<
                std::add_pointer_t<T>,
                MapViewOfFileDeleter>;

        struct GDIHandleDeleter
        {
            void operator()(HGDIOBJ h_gdi_object) const noexcept
            {
                if (h_gdi_object)
                {
                    ::DeleteObject(h_gdi_object);
                }
            }
        };
        using UniqueHdc = Utils::RAIIWrapper<HDC, GDIHandleDeleter>;

        struct GLResourceHandleDeleter
        {
            void operator()(HGLRC h_gl_rc) const noexcept
            {
                if (h_gl_rc)
                {
                    ::wglDeleteContext(h_gl_rc);
                }
            }
        };
        using UniqueGlRc = Utils::RAIIWrapper<HGLRC, GLResourceHandleDeleter>;

        template <class T>
        bool OverrideLimitedMemory(void* p_limited_memory, const T* p_data)
        {
            constexpr auto size = sizeof(T);
            constexpr DWORD enable_write = 1;
            DWORD old_protect{0};
            if (::VirtualProtect(
                    p_limited_memory,
                    size,
                    enable_write,
                    &old_protect))
            {
                auto from = p_data;
                auto to = p_limited_memory;
                std::memcpy(to, from, size);
                ::VirtualProtect(
                    p_limited_memory,
                    size,
                    old_protect,
                    nullptr);
                return true;
            }
            return false;
        }

        bool QueryProcessId(const wchar_t& w_process_name, DWORD* p_out_pid) FAST_CAPTURE_NOEXCEPT
        {
            UniqueHandleInvalidMinusOne h_snapshot =
                ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            const std::wstring_view process_name{&w_process_name};
            PROCESSENTRY32W pe32w{};
            pe32w.dwSize = sizeof(PROCESSENTRY32W);

            if (!Process32FirstW(h_snapshot.Get(), &pe32w))
            {
                goto on_error;
            }
            do
            {
                const std::wstring_view pe32_name{pe32w.szExeFile};
                if (process_name == pe32_name)
                {
                    *p_out_pid = pe32w.th32ProcessID;
                    return true;
                }
            } while (::Process32NextW(h_snapshot.Get(), &pe32w));

        on_error:
            *p_out_pid = -1;
            return false;
        }

        inline bool CopyDataToRemote(HANDLE h_process, const void* from, void* to, const SIZE_T size) noexcept
        {
            SIZE_T copied_size = 0;
            ::WriteProcessMemory(
                h_process,
                to,
                from,
                size,
                &copied_size);
            return copied_size == size;
        }
        template <class T>
        bool CopyDataToRemote(HANDLE h_process, const T* from, void* to) noexcept
        {
            constexpr SIZE_T size = sizeof(T);
            return CopyDataToRemote(h_process, from, to, size);
        }
        template <class T>
        bool ReadDataFromRemote(HANDLE h_process, const T* from, T* to) noexcept
        {
            constexpr SIZE_T size = sizeof(T);
            SIZE_T read_size = 0;
            ::ReadProcessMemory(
                h_process,
                from,
                to,
                size,
                &read_size);
            return read_size == size;
        }
        namespace Details
        {
            template <class T>
            struct RemoteMemoryDeleter
            {
                void operator()(auto&& memory_info) const noexcept
                {
                    auto [p_remote_memory, h_process] = memory_info;
                    if (p_remote_memory != nullptr)
                    {
                        ::VirtualFreeEx(h_process,
                                        p_remote_memory,
                                        sizeof(T),
                                        MEM_RELEASE);
                    }
                }
            };
            struct RemoteMemoryVerifier
            {
                bool operator()(auto&& memory_info) const noexcept
                {
                    auto p_remote_memory = std::get<0>(memory_info);
                    return p_remote_memory == nullptr;
                }
            };
            template <class T>
            using UniqueRemoteMemoryPtrBase = Utils::RAIIWrapper<
                std::tuple<T*, HANDLE>,
                RemoteMemoryDeleter<T>,
                RemoteMemoryVerifier>;
        }
        template <class T>
        class UniqueRemoteMemoryPtr : public Details::UniqueRemoteMemoryPtrBase<T>
        {
            using Base = Details::UniqueRemoteMemoryPtrBase<T>;

        public:
            using Base::Base;
            T* Get() const noexcept
            {
                return std::get<0>(Base::GetRef());
            }
        };
        template <class T>
        auto MakeUniqueRemoteMemoryPtr(HANDLE h_process)
            -> UniqueRemoteMemoryPtr<T>
        {
            return {std::make_tuple(
                reinterpret_cast<T*>(
                    ::VirtualAllocEx(
                        h_process,
                        nullptr,
                        sizeof(T),
                        MEM_COMMIT,
                        PAGE_READWRITE)),
                h_process)};
        }

        struct HwndDeleter
        {
            void operator()(HWND hwnd) const noexcept
            {
                if (hwnd != nullptr)
                {
                    ::SendMessageW(hwnd, WM_CLOSE, 0, 0);
                }
            }
        };
        using UniqueHwnd = Utils::RAIIWrapper<HWND, HwndDeleter>;

        inline FastCaptureErrorCode GetCurrentModuleAndAddCurrentModuleRefCount(HMODULE* p_out_current_module) noexcept
        {
            if (!::GetModuleHandleExA(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                    reinterpret_cast<LPCSTR>(&GetCurrentModuleAndAddCurrentModuleRefCount),
                    p_out_current_module))
            {
                return Windows::MakeError(FAST_CAPTURE_E_GET_SELF_DLL_HANDLE_FAILED);
            }
            return FastCaptureMakeSuccessValue();
        }
        struct FreeLibraryAndExitThreadGuardDeleter
        {
            void operator()(HMODULE h_module) const noexcept
            {
                if (h_module != nullptr)
                {
                    ::FreeLibraryAndExitThread(h_module, 0);
                }
            }
        };
        using FreeLibraryAndExitThreadGuard = Utils::RAIIWrapper<HMODULE, FreeLibraryAndExitThreadGuardDeleter>;
    }
}

#endif // FAST_CAPTURE_UTILS_WINDOWS_UTILS_WINDOWS_HPP
