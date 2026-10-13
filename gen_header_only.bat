echo #ifndef _SR_IMPL_H_ > %~dp0/include/SR_impl.h
echo #define _SR_IMPL_H_ >> %~dp0/include/SR_impl.h
echo: >> %~dp0/include/SR_impl.h 

echo //USAGE - Define __SR_IMPL__ in ONE source file only >> %~dp0/include/SR_impl.h
echo #ifdef __SR_IMPL__ >> %~dp0/include/SR_impl.h

type "%~dp0\src\SR_renderer.c" "%~dp0\src\SR_matrix.c" >> %~dp0/include/SR_impl.h

echo: >> %~dp0/include/SR_impl.h
echo #endif >> %~dp0/include/SR_impl.h
echo #endif >> %~dp0/include/SR_impl.h