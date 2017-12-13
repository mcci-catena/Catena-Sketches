#ifdef ARDUINO_ARCH_SAMD
# include <Catena4450.h>
# include <CatenaRTC.h>
  using ThisCatena = McciCatena::Catena4450;
# include <delay.h>
#else
# ifdef ARDUINO_ARCH_STM32
# include <Catena4551.h>
# include <CatenaStm32L0Rtc.h>
 using ThisCatena = McciCatena::Catena4551;
# else
#   error Architecture not supported
# endif
#endif
