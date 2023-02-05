此DLL中应当声明、实现并导出

    实现定义 FastCaptureInitInjectDll(实现定义) noexcept

    实现定义 FastCaptureDestroyDll(实现定义) noexcept

    这以上两个函数。

    其中，

    InitInjectDll负责初始化DLL内部的变量

    DestroyDll负责清理DLL内部的变量，并且主动卸载DLL

    有关实现定义的细节，参照./平台/FastCaptureInjectDll.h文件
