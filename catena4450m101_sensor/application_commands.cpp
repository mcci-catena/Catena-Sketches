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

using namespace McciCatena;

/* library routine -- but not in library yet! */
static cCommandStream::CommandStatus
getuint32(
	int argc,
	char **argv,
	int iArg,
	uint32_t& result,
	uint32_t uDefault
	);

static cCommandStream::CommandStatus
getuint32(
	int argc,
	char **argv,
	int iArg,
	uint32_t& result,
	uint32_t uDefault
	)
	{
	bool fOverflow;

	// substitute default if needed
	if (iArg >= argc)
		{
		result = uDefault;
		return cCommandStream::CommandStatus::kSuccess;
		}

	const char * const pArg = argv[iArg];
	size_t nArg = std::strlen(pArg);

	size_t const nc = McciAdkLib_BufferToUint32(
				pArg,
				nArg,
				0,      // use C rules for the number base
				&result,
				&fOverflow
				);

	if (nc == 0 || nc != nArg || fOverflow)
		return cCommandStream::CommandStatus::kError;
	else
		return cCommandStream::CommandStatus::kSuccess;
	}

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
                pThis->printf("%u ms\n", (unsigned) txPeriodSeconds);
                return cCommandStream::CommandStatus::kSuccess;
                }
        else
                {
                // convert argv[1] to unsigned
                cCommandStream::CommandStatus status;
                uint32_t newTxPeriod;

                // get arg 1 as tx period; default is irrelevant because
                // we know that we have argv[1] from above.
                status = getuint32(argc, argv, 1, newTxPeriod, 0);

                if (status != cCommandStream::CommandStatus::kSuccess)
                        return status;

                txPeriodSeconds = newTxPeriod;
                return status;
                }
        }
