#ifndef FAST_CAPTURE_INJECT_DLL_DLL_DATA_HPP
#define FAST_CAPTURE_INJECT_DLL_DLL_DATA_HPP

#include "FastCaptureDef.h"
#include <string>
#include "../FastCaptureInjectDllDef.h"
#include "../../Utils/Windows/UtilsWindows.hpp"

FAST_CAPTURE_NAMESPACE
{
    class DllData
    {
    public:
        /**
         * @brief 此变量在FastCaptureInitInjectDll中初始化，之后不会被修改
         *
         */
        Windows::UniqueHandleInvalidNULL h_capture_descriptor_{};
        /**
         * @brief 此变量在FastCaptureInitInjectDll中初始化，之后不会被修改
         *
         */
        Windows::UniqueMapViewOfFile<CaptureDescriptor> p_capture_descriptor_{};
        /**
         * @brief 此变量在截图大小出现变化时被PrepareCaptureImage修改
         *
         */
        Windows::UniqueHandleInvalidNULL h_capture_image_{};
        /**
         * @brief 此变量在截图大小出现变化时被PrepareCaptureImage修改
         *
         */
        Windows::UniqueMapViewOfFile<CaptureImage> p_capture_data_{};
        /**
         * @brief 此变量在FastCaptureInitInjectDll中初始化，之后不会被修改
         *
         */
        std::wstring shared_memory_name_prefix_{};

    private:
        DllData() = default;
        ~DllData() = default;

    public:
        static DllData& GetInstance() noexcept
        {
            static DllData result;
            return result;
        }
    };
}

#endif // FAST_CAPTURE_INJECT_DLL_DLL_DATA_HPP
