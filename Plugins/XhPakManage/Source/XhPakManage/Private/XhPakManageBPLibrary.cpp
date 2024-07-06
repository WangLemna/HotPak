// Copyright Epic Games, Inc. All Rights Reserved.

#include "XhPakManageBPLibrary.h"
#include "XhPakManage.h"

UXhPakManageBPLibrary::UXhPakManageBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

TArray<FString> UXhPakManageBPLibrary::ReadFileToStringArray(const FString& FilePath)
{
	TArray<FString> Content;
	if (FPaths::FileExists(FilePath))
	{
		FFileHelper::LoadFileToStringArray(Content, *FilePath);
	}
	return Content;
}

