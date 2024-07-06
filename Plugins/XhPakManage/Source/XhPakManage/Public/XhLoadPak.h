// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "XhLoadPak.generated.h"
//Content, UMETA(DisplayName = "相对路径 .../Windows/{项目名称}/Content"),
class IPlatformFile;
class FPakPlatformFile;
//路径类别
UENUM(BlueprintType)
enum class XhLoadPakDir : uint8
{
	//相对路径 .../Windows/{项目名称}/Content
	Content, 
	//绝对路径
	Absolute,
};
//加载Pak资源的类别
UENUM(BlueprintType)
enum class XhLoadPakSource : uint8
{
	//以uasset结尾的资源
	Suffix_uasset,
	//以umap结尾的资源(关卡资源)
	Suffix_umap,
	//以'M_'开头,以umap结尾的资源(关卡资源)
	Prefix_M,
	//以'BP_'开头,以uasset结尾的资源(一般是蓝图)
	Prefix_BP,
	//以'T_'开头,以uasset结尾的资源(一般是贴图)
	Prefix_T,
	//以'SM_'开头,以uasset结尾的资源(一般是静态网格体)
	Prefix_SM,
	//所有文件
	All,
};
//关卡的参数，因为关卡需要队列加载，不能同步加载
struct XhLevelParam 
{
	TArray<FString> LevelNamesParam;
	bool bIsLoadParam;
	int32 CurrentIndex;
public:
	XhLevelParam(TArray<FString> InLevelNamesParam, bool InIsLoadParam)
	{
		LevelNamesParam = InLevelNamesParam;
		bIsLoadParam = InIsLoadParam;
		CurrentIndex = 0;
	}
	FString ToString()
	{
		FString Result;
		for (const FString& temp : LevelNamesParam)
		{
			Result += temp + " ";
		}
		Result += bIsLoadParam ? TEXT("被加载") : TEXT("被卸载");
		return Result;
	}
	bool operator==(const XhLevelParam& InXhLevelParam) const
	{
		return LevelNamesParam == InXhLevelParam.LevelNamesParam && bIsLoadParam == InXhLevelParam.bIsLoadParam;
	}
};
UCLASS()
class XHPAKMANAGE_API AXhLoadPak : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AXhLoadPak();
	/*
	* FPaths::ProjectContentDir()  打包后的Content目录  =>  {之前的路径}/Windows/{项目名称}/Content/
	* 加载pak文件，输出为资源名称
	*/
	UFUNCTION(BlueprintCallable, meta = (Keywords = "XhLoadPak"), Category = "XhPakManage")
	void XhLoadPak(TArray<FString>& OutResults, const FString& InPakPath, XhLoadPakDir InLoadPakDir = XhLoadPakDir::Content, XhLoadPakSource InLoadPakSource = XhLoadPakSource::Suffix_umap, bool bIsPluginContent = false);

	//true 代表加载， false 代表卸载
	UFUNCTION(BlueprintCallable, meta = (Keywords = "XhLoadStreamLevels"), Category = "XhPakManage")
	void XhLoadStreamLevels(const TArray<FString>& LevelNames, bool bIsLoad = true);
	//创建关卡流
	FString CreateStreamInstance(UWorld* World, const FString& LongPackageName);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	FPakPlatformFile* NewPakPlatform;
	IPlatformFile* OldPlatform;
protected:
	TArray< XhLevelParam > ToltalLoadLevels;
	bool bIsLoadingLevel;


	TArray<FString> AlreadyLoadPaks;
};

class FLevelLoadedLatentAction : public FPendingLatentAction
{
public:
	TFunction<void()> SuccessCallback;
	FString Description;

	FLevelLoadedLatentAction(TFunction<void()> InSuccessCallback, const FString& InDescription)
		: SuccessCallback(InSuccessCallback), Description(InDescription)
	{
	}
	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		if (SuccessCallback)
		{
			SuccessCallback();
		}
		Response.DoneIf(true);
	}
#if WITH_EDITOR
	virtual FString GetDescription() const
	{
		return Description;
	}
#endif
};

