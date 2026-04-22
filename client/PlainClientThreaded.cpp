#include "PlainClientApp.h"

#include "ThreadedQosRuntime.h"

int PlainClientApp::RunThreadedMode()
{
	ThreadedQosRuntime runtime(*this);
	return runtime.Run();
}
