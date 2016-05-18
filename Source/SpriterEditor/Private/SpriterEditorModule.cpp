
#include "SpriterEditorPrivatePCH.h"

DEFINE_LOG_CATEGORY(LogSpriterImporter);

class FSpriterEditorModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FSpriterEditorModule, SpriterEditor);
