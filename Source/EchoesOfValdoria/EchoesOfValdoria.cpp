// Copyright Epic Games, Inc. All Rights Reserved.

#include "EchoesOfValdoria.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Containers/Ticker.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

/** Owner (turn 53): test runs (-ValdoriaBackground) must not steal the mouse or the focused window on monitor 1.
 *  The window the owner was using is remembered at startup and given the focus back for the first seconds,
 *  whenever the game window takes it. */
class FEchoesOfValdoriaModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
#if PLATFORM_WINDOWS
		if (!FParse::Param(FCommandLine::Get(), TEXT("ValdoriaBackground")))
		{
			return;
		}
		HWND Previous = ::GetForegroundWindow();
		DWORD PreviousPid = 0;
		::GetWindowThreadProcessId(Previous, &PreviousPid);
		if (!Previous || PreviousPid == ::GetCurrentProcessId())
		{
			return;
		}
		const double Until = FPlatformTime::Seconds() + 40.0; // engine start + map load
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Previous, Until](float)
		{
			HWND Now = ::GetForegroundWindow();
			DWORD Pid = 0;
			::GetWindowThreadProcessId(Now, &Pid);
			if (Pid == ::GetCurrentProcessId() && ::IsWindow(Previous))
			{
				::SetForegroundWindow(Previous);
			}
			return FPlatformTime::Seconds() < Until;
		}), 0.1f);
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FEchoesOfValdoriaModule, EchoesOfValdoria, "EchoesOfValdoria" );

DEFINE_LOG_CATEGORY(LogEchoesOfValdoria)