#pragma once

#include <cstdlib>

// Reads an environment variable only when MEDIASOUP_TEST_HOOKS is defined.
// Returns nullptr in production builds, suppressing test-only env reads.
inline const char* loadTestHookEnv(const char* name)
{
#ifdef MEDIASOUP_TEST_HOOKS
	return std::getenv(name);
#else
	(void)name;
	return nullptr;
#endif
}
