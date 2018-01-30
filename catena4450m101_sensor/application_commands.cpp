//
// at least with Visual Micro, it's necessary to hide these function bodies
// from the Arduino environment. Arduino tries to insert prototypes for these
// functions at the start of the file, and doesn't handle the return type
// properly.
//
// By putting in a #included CPP file, we avoid the deep scan, and thereby
// avoid the problem.
//
#include <Catena_CommandStream.h>
#include <cstring>
#include <mcciadk_baselib.h>
#include <CatenaBase.h>

using namespace McciCatena;

extern unsigned gTxCycle;

#if ! defined(CATENA_ARDUINO_PLATFORM_VERSION)
# error MCCI Catena Arduino Platform is out of date. Check CATENA_ARDUINO_PLATFORM_VERSION
#elif (CATENA_ARDUINO_PLATFORM_VERSION < CATENA_ARDUINO_PLATFORM_VERSION_CALC(0, 14, 0, 50))
# error MCCI Catena Arduino Platform is out of date. Check CATENA_ARDUINO_PLATFORM_VERSION
#endif

/* process "application tx-period" -- without args, display, with an arg set the value */
// argv[0] is "tx-period"
// argv[1] if present is the new value
cCommandStream::CommandStatus
setTransmitPeriod(
	cCommandStream *pThis,
	void *pContext,
	int argc, 
	char **argv
	)
	{
        if (argc > 2)
                return cCommandStream::CommandStatus::kInvalidParameter;

        if (argc < 2)
                {
                pThis->printf("%u s\n", (unsigned) gTxCycle);
                return cCommandStream::CommandStatus::kSuccess;
                }
        else
                {
                // convert argv[1] to unsigned
                cCommandStream::CommandStatus status;
                uint32_t newTxPeriod;

                // get arg 1 as tx period; default is irrelevant because
                // we know that we have argv[1] from above.
                status = cCommandStream::getuint32(argc, argv, 1, /* radix */ 0, newTxPeriod, 0);

                if (status != cCommandStream::CommandStatus::kSuccess)
                        return status;

                gTxCycle = newTxPeriod;
                return status;
                }
        }
