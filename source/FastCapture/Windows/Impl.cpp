#include "Impl.h"
#include <cstdint>
#include <cstring>
#include <string_view>
#include <memory>
#include <tuple>
#include <type_traits>
#include <bit>
#include "FastCaptureDef.h"
#include "../../Utils/Utils.hpp"
#include "../../Utils/Windows/UtilsWindows.hpp"
#include "../../FastCaptureInjectDll/Windows/FastCaptureInjectDll.h"

FAST_CAPTURE_NAMESPACE
{
    namespace Windows
    {
        template <Utils::is_wchar_array WC>
        FastCaptureErrorCode InstallInjectDll(HANDLE h_process, WC& dll_name, const std::wstring& shared_memory_name_prefix) noexcept
        {
            using DllName = std::remove_cv_t<WC>;

            auto remote_dll_name = Windows::MakeUniqueRemoteMemoryPtr<DllName>(h_process);
            if (!remote_dll_name.IsInvalid())
            {
                return MakeError(FAST_CAPTURE_E_CREATE_REMOTE_MEMORY_FAILED);
            }
            auto from = &dll_name;
            auto to = remote_dll_name.Get();
            if (!CopyDataToRemote(h_process, from, to))
            {
                return MakeError(FAST_CAPTURE_E_CREATE_REMOTE_MEMORY_FAILED);
            }

            UniqueHandleInvalidNULL h_load_dll_thread = ::CreateRemoteThread(
                h_process,
                nullptr,
                0,
                std::bit_cast<LPTHREAD_START_ROUTINE>(&::LoadLibraryW),
                remote_dll_name.Get(),
                0,
                nullptr);
            if (h_load_dll_thread.IsInvalid())
            {
                return MakeError(FAST_CAPTURE_E_CREATE_INJECT_THREAD_FAILED);
            }
            switch (auto wait_dll_loading_result = ::WaitForSingleObject(h_load_dll_thread.Get(), 4000))
            {
            case WAIT_OBJECT_0:
                break;
            case WAIT_TIMEOUT:
                return MakeError(FAST_CAPTURE_E_WAIT_INJECT_THREAD_TIMEOUT);
            case WAIT_FAILED:
                return MakeError(FAST_CAPTURE_E_WAIT_INJECT_FAILED);
            default:
                return MakeError(FAST_CAPTURE_E_WAIT_RESULT_UNEXPECTED);
            }

            FastCaptureInjectDllInitDesc inject_dll_init_desc{};
            if (shared_memory_name_prefix.length() >= std::size(inject_dll_init_desc.share_memory_name_prefix))
            {
                return Utils::MakeError(1);
            }
            auto remote_inject_dll_init_desc = MakeUniqueRemoteMemoryPtr<FastCaptureInjectDllInitDesc>(h_process);
            if (remote_inject_dll_init_desc.IsInvalid())
            {
                return MakeError(FAST_CAPTURE_E_CREATE_REMOTE_MEMORY_FAILED);
            }
            if (!CopyDataToRemote(
                    h_process,
                    shared_memory_name_prefix.data(),
                    &(*remote_inject_dll_init_desc.Get()).share_memory_name_prefix,
                    shared_memory_name_prefix.length() * sizeof(wchar_t)))
            {
                return MakeError(FAST_CAPTURE_E_COPY_INJECT_DLL_INIT_DATA_FAILED);
            }
            UniqueHandleInvalidNULL h_init_dll_thread = ::CreateRemoteThread(
                h_process,
                nullptr,
                0,
                &FastCaptureInitInjectDll,
                remote_inject_dll_init_desc.Get(),
                0,
                nullptr);
            if (h_init_dll_thread.IsInvalid())
            {
                return MakeError(FAST_CAPTURE_E_CREATE_INJECT_THREAD_FAILED);
            }
            switch (auto wait_dll_init_result = ::WaitForSingleObject(h_init_dll_thread.Get(), 4000))
            {
            case WAIT_OBJECT_0:
                break;
            case WAIT_TIMEOUT:
                return MakeError(FAST_CAPTURE_E_INIT_INJECT_DLL_TIMEOUT);
            case WAIT_FAILED:
                return MakeError(FAST_CAPTURE_E_INIT_INJECT_DLL_FAILED);
            default:
                return MakeError(FAST_CAPTURE_E_WAIT_RESULT_UNEXPECTED);
            }
            FastCaptureErrorCode init_dll_result = FastCaptureMakeSuccessValue();
            if (!ReadDataFromRemote(h_process, &(*remote_inject_dll_init_desc.Get()).init_result, &init_dll_result))
            {
                return MakeError(FAST_CAPTURE_E_GET_INIT_INJECT_DLL_RESULT_FAILED);
            }
            return init_dll_result;
        }

        FastCaptureErrorCode InjectProcessImpl(
            const wchar_t& w_process_name,
            const std::wstring& shared_memory_name_prefix) FAST_CAPTURE_NOEXCEPT
        {
            DWORD pid;
            if (!QueryProcessId(w_process_name, &pid))
            {
                return MakeError(FAST_CAPTURE_E_INVALID_ARGUMENT);
            }
            Windows::UniqueHandleInvalidNULL h_process = ::OpenProcess(
                PROCESS_ALL_ACCESS,
                FALSE,
                pid);
            return InstallInjectDll(
                h_process.Get(), L"FastCaptureInjectDll.dll", shared_memory_name_prefix);
        }
    }
}
