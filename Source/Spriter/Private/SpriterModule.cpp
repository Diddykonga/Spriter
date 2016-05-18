
#include "SpriterPrivatePCH.h"

DEFINE_LOG_CATEGORY(LogSpriterImporter);

class FSpriterModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FSpriterModule, Spriter);
