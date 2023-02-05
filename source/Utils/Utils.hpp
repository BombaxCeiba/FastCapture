#ifndef FAST_CAPTURE_UTILS_UTILS_HPP
#define FAST_CAPTURE_UTILS_UTILS_HPP
#include "FastCaptureDef.h"
#include <cstring>
#include <utility>
#include <memory>
#include <cstring>

FAST_CAPTURE_NAMESPACE
{
    namespace Utils
    {
        template <class T>
        concept is_char_array =
            std::is_bounded_array_v<std::remove_cvref_t<T>> && std::is_same_v<std::remove_pointer_t<std::decay_t<T>>, std::remove_pointer_t<std::decay_t<decltype("")>>>;
        template <class T>
        concept is_wchar_array =
            std::is_bounded_array_v<std::remove_cvref_t<T>> && std::is_same_v<std::remove_pointer_t<std::decay_t<T>>, std::remove_pointer_t<std::decay_t<decltype(L"")>>>;

        template <class T>
        void* MemSet(T* pointer, int ch = 0)
        {
            return std::memset(pointer, ch, sizeof(T));
        }
        template <class T>
        void* CopyData(const T* from, void* to)
        {
            return std::memcpy(to, from, sizeof(T));
        }
        template <class T, class... Args>
        void Emplace(T& object, Args&&... args)
        {
            ::new (std::addressof(object)) T(std::forward<Args>(args)...);
        }
        template <class T>
        void Destroy(T& object)
        {
            object.~T();
        }
        /**
         * @brief 默认的资源验证器，当输入=nullptr时返回true
         *
         */
        struct DefaultVerifier
        {
            bool operator()(auto&& value) const noexcept
            {
                return value == nullptr;
            }
        };
        /**
         * @brief 空类。在被使用时，void会被映射为它
         *
         */
        struct EmptyStruct
        {
        };
        /**
         * @brief RAII包装类
         *
         * @tparam T 可默认构造的类型
         * @tparam Dtor T实例化对象析构时执行的可调用对象类型
         * @tparam Verifier 验证T实例化对象是否不可用的可调用对象类型
         */
        template <
            class T,
            class Dtor = std::default_delete<T>,
            class Verifier = DefaultVerifier>
        class RAIIWrapper
        {
        public:
            constexpr static bool IsDtorNotAPointer() noexcept
            {
                return !std::is_pointer_v<Dtor>;
            }
            constexpr static bool IsVerifierNotAPointer() noexcept
            {
                return !std::is_pointer_v<Verifier>;
            }
            RAIIWrapper(Dtor dtor)
                requires requires {
                !IsDtorNotAPointer();
                IsVerifierNotAPointer(); }
                : dtor_(dtor)
            {
            }
            RAIIWrapper(Dtor dtor = {})
                requires requires {
                IsDtorNotAPointer();
                IsVerifierNotAPointer(); }
                : dtor_(dtor)
            {
            }
            template <class Object>
                requires requires {
                             std::is_convertible_v<Object, T>;
                             IsDtorNotAPointer(); }
            RAIIWrapper(Object&& object, Dtor dtor = {})
                : value_(std::forward<Object>(object)),
                  dtor_{dtor}, verifier_()
            {
            }
            RAIIWrapper(const RAIIWrapper&) = delete;
            RAIIWrapper& operator=(const RAIIWrapper&) = delete;
            RAIIWrapper(RAIIWrapper&& other) noexcept
                : value_(std::exchange(other.value_, nullptr)),
                  dtor_(std::exchange(other.dtor_, this->dtor_)),
                  verifier_(std::exchange(other.verifier_, this->verifier_))
            {
            }
            RAIIWrapper& operator=(RAIIWrapper&& other) noexcept
            {
                std::swap(this->value_, other.value_);
                std::swap(this->dtor_, other.dtor_);
                std::swap(this->verifier_, other.verifier_);
                return *this;
            }
            ~RAIIWrapper() noexcept
            {
                dtor_(value_);
            }
            T Get() const noexcept
            {
                return value_;
            }
            bool IsInvalid() const noexcept
            {
                return verifier_(value_);
            }

        protected:
            const T& GetRef() const noexcept
            {
                return value_;
            }

        private:
            T value_;
            [[no_unique_address]] Dtor dtor_;
            [[no_unique_address]] Verifier verifier_;
        };

        inline FastCaptureErrorCode MakeError(std::uint16_t fast_capture_error_code)
        {
            return {
                fast_capture_error_code,
                FAST_CAPTURE_ERROR_TYPE_DEFAULT,
                0};
        }

        inline bool IsOk(FastCaptureErrorCode fast_capture_error_code) noexcept
        {
            return fast_capture_error_code.error_code == FAST_CAPTURE_S_OK;
        }
    }
}

#endif // FAST_CAPTURE_UTILS_UTILS_HPP
