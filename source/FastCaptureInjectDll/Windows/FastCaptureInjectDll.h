#ifndef FAST_CAPTURE_INJECT_DLL_SERVER_WINDOWS_IMPL_HEADER_HPP
#define FAST_CAPTURE_INJECT_DLL_SERVER_WINDOWS_IMPL_HEADER_HPP

#include "FastCaptureDef.h"
#include <Windows.h>

struct FastCaptureInjectDllInitDesc
{
    wchar_t share_memory_name_prefix[128] = {};
    FastCaptureErrorCode init_result = FastCaptureMakeSuccessValue();
};

extern "C"
{
    /**
     * @brief 被来自远程创建的线程调用，用于初始化dll变量并hook必须的函数。
     *  若出错，需要调用::GetLastError获得更多信息
     *
     * @param lpThreadParameter 类型为InjectDllInitDesc*，要指定的共享内存的前缀，以及init的返回值
     *  注意：
     *      捕获的图片的信息的共享内存名称为
     *          std::wstring(L"Global\\") + std::wstring(lpThreadParameter) + std::wstring(L"Descriptor")"
     *      捕获的图片的共享内存的名称为
     *          std::wstring(L"Global\\") + std::wstring(lpThreadParameter)
     * @return FAST_CAPTURE_EXPORT 返回值一定可以被转换为有效的 FastCaptureErrorCode
     */
    FAST_CAPTURE_EXPORT
    DWORD WINAPI FastCaptureInitInjectDll(LPVOID lpThreadParameter) FAST_CAPTURE_NOEXCEPT;

    /**
     * @brief 被来自远程创建的线程调用，用于卸载hook并清理dll变量，
     *  若出错，需要调用::GetLastError获得更多信息
     *
     * @param lpThreadParameter
     * @return FAST_CAPTURE_EXPORT 必定返回0
     */
    FAST_CAPTURE_EXPORT
    DWORD WINAPI FastCaptureDestroyDll(LPVOID lpThreadParameter) FAST_CAPTURE_NOEXCEPT;
}

#endif // FAST_CAPTURE_INJECT_DLL_SERVER_WINDOWS_IMPL_HEADER_HPP
