#include <Catena.h>
using ThisCatena = McciCatena::Catena;

#ifdef ARDUINO_ARCH_SAMD
# include <CatenaRTC.h>
# include <delay.h>
#elif defined(ARDUINO_ARCH_STM32)
# include <CatenaStm32L0Rtc.h>
#endif
