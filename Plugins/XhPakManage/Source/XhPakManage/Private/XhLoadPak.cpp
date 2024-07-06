// Fill out your copyright notice in the Description page of Project Settings.


#include "XhLoadPak.h"
#include "IPlatformFilePak.h"
#include "HAL/PlatformFilemanager.h"
#include "Runtime/Engine/Classes/Engine/StreamableManager.h"
#include "Runtime/Engine/Classes/Engine/AssetManager.h"
#include "Runtime/Engine/Classes/Engine/StaticMeshActor.h"
#include "Kismet/KismetStringLibrary.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AXhLoadPak::AXhLoadPak()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bIsLoadingLevel = false;
	OldPlatform = &FPlatformFileManager::Get().GetPlatformFile();
	NewPakPlatform = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(FPakPlatformFile::GetTypeName()));
	AlreadyLoadPaks.Empty();
}

#define XH_GET_UASSET_NAME(prefix,suffix) FString NewFileName = Filename;\
FString File = FPaths::GetBaseFilename(Filename);\
NewFileName = RootPath + CleanFilename;\
NewFileName.ReplaceInline(TEXT("uasset"), *File);\
NewFileName.Append(TEXT(suffix));\
NewFileName = UKismetStringLibrary::Concat_StrStr(TEXT(prefix), NewFileName);\
OutResults.AddUnique(NewFileName);


void AXhLoadPak::XhLoadPak(TArray<FString>& OutResults, const FString& InPakPath, XhLoadPakDir InLoadPakDir/* = XhLoadPakDir::Content*/, XhLoadPakSource InLoadPakSource/* = XhLoadPakSource::Suffix_umap*/, bool bIsPluginContent/* = false*/)
{
	//判断是否是在编辑器
	if (GetWorld()->WorldType != EWorldType::Game)
	{
		return;
	}
	OutResults.Empty();
	FString PakFullPath;
	switch (InLoadPakDir)
	{
	case XhLoadPakDir::Content:
		PakFullPath = FPaths::ProjectContentDir() + InPakPath;
		break;
	case XhLoadPakDir::Absolute:
		PakFullPath = InPakPath;
		break;
	default:
		break;
	}
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*PakFullPath))
	{
		UE_LOG(LogTemp, Log, TEXT(" 没有该路径  %s"), *PakFullPath);
		return;
	}
	else
	{
		if (AlreadyLoadPaks.Contains(PakFullPath))
		{
			UE_LOG(LogTemp, Log, TEXT("%s 已经被加载"), *PakFullPath);
			return;
		}
	}

	TRefCountPtr<FPakFile> TmpPak = new FPakFile(OldPlatform, *PakFullPath, false);
	FString OldPakMountPoint = TmpPak->GetMountPoint();
	//UE_LOG(LogTemp, Log, TEXT(" OldPakMountPoint为  %s"), *OldPakMountPoint);// D:/UEProject/Xh/HotPak/Saved/Cooked/Windows/HotPak/Plugins/XhTool/Content/ssss/
	int32 ContentPos = -1;
	FString RootPath;
	if (bIsPluginContent)
	{
		ContentPos = OldPakMountPoint.Find("Plugins/");
		RootPath = OldPakMountPoint.RightChop(ContentPos + 7);//  /XhTool/Content/ssss/
		// 查看log日志就可以发现他是直接省略掉Content文件夹了，所以在此也要去掉
		FString ls = RootPath.Left(RootPath.Find("Content/"));//  /XhTool/    
		FString rs = RootPath.RightChop(RootPath.Find("Content/") + 8);//  ssss/
		RootPath = ls + rs;//   /XhTool/ssss/

	}
	else
	{
		ContentPos = OldPakMountPoint.Find("Content/");
		// 查看log日志就可以发现他是直接省略掉Content文件夹了，所以在此也要去掉
		FString rs = OldPakMountPoint.RightChop(ContentPos + 8);//  ssss/
		RootPath = "/Game/" + rs;
	}
	//UE_LOG(LogTemp, Log, TEXT(" RootPath为  %s"), *RootPath);
	FString NewMountPoint = OldPakMountPoint.RightChop(ContentPos);// Content/{自己的路径}    Plugins/{自己的路径}
	//UE_LOG(LogTemp, Log, TEXT(" NewMountPoint为  %s"), *NewMountPoint);//   Plugins/XhTool/Content/ssss/
	FString ProjectPath = FPaths::ProjectDir();//   ../../../HotPak/
	//挂载路径
	FString NewMountPath = ProjectPath + NewMountPoint;// ../../../HotPak/Content/{自己的路径}    ../../../Plugins/{自己的路径}
	//NewMountPath = FPaths::Combine(FPaths::ProjectDir(), NewMountPath);
	//UE_LOG(LogTemp, Log, TEXT(" NewMountPath为  %s"), *NewMountPath);// ../../../HotPak/Plugins/XhTool/Content/ssss/

	TmpPak->SetMountPoint(*NewMountPath);
	AlreadyLoadPaks.Add(PakFullPath);
	if (NewPakPlatform->Mount(*PakFullPath, 1, *NewMountPath))
	{
		TArray<FString> FoundFilenames;
		OldPakMountPoint = TmpPak->GetMountPoint();
		TmpPak->FindPrunedFilesAtPath(FoundFilenames, *TmpPak->GetMountPoint(), true, false, true);
		if (FoundFilenames.Num() > 0)
		{
			switch (InLoadPakSource)
			{
			case XhLoadPakSource::Suffix_uasset:
			{
				for (FString& Filename : FoundFilenames)
				{
					if (Filename.EndsWith(TEXT(".uasset")))
					{
						OutResults.AddUnique(Filename);
					}
				}
			}
				break;
			case XhLoadPakSource::Suffix_umap:
			{
				for (FString& Filename : FoundFilenames)
				{
					FString CleanFilename = FPaths::GetCleanFilename(Filename);
					UE_LOG(LogTemp, Log, TEXT(" 文件名为  %s"), *CleanFilename);
					if (Filename.EndsWith(TEXT(".umap")))
					{
						Filename.RemoveFromEnd(TEXT(".umap"));
						FString ls;
						FString TempLevelName;
						Filename.Split(TEXT("/"), &ls, &TempLevelName, ESearchCase::Type::CaseSensitive, ESearchDir::FromEnd);
						
						FString LevelPakagePath = RootPath + TempLevelName;//  /Game/{自己的路径}/{关卡名称}
						//FString LevelPakagePath = "/Game/" + NewMountPoint.RightChop(8) + TempLevelName;//  /Game/{自己的路径}/{关卡名称}
						//调用函数，将关卡名添加到流关卡中
						CreateStreamInstance(GetWorld(), LevelPakagePath);
						OutResults.AddUnique(LevelPakagePath);
					}
				}
			}
				break;
			case XhLoadPakSource::Prefix_M:
			{
				for (FString& Filename : FoundFilenames)
				{
					FString CleanFilename = FPaths::GetCleanFilename(Filename);
					UE_LOG(LogTemp, Log, TEXT(" 文件名为  %s"), *CleanFilename);
					if (CleanFilename.EndsWith(TEXT(".umap")))
					{
						Filename.RemoveFromEnd(TEXT(".umap"));
						FString ls;
						FString TempLevelName;
						Filename.Split(TEXT("/"), &ls, &TempLevelName, ESearchCase::Type::CaseSensitive, ESearchDir::FromEnd);
						//FString LevelPakagePath = "/Game/" + NewMountPoint.RightChop(8) + TempLevelName;//  /Game/{自己的路径}/{关卡名称}
						FString LevelPakagePath = RootPath + TempLevelName;//  /Game/{自己的路径}/{关卡名称}
						//调用函数，将关卡名添加到流关卡中
						CreateStreamInstance(GetWorld(), LevelPakagePath);
						if (CleanFilename.StartsWith(TEXT("M_")))
						{
							OutResults.AddUnique(LevelPakagePath);
						}
					}
				}
			}
			break;
			case XhLoadPakSource::Prefix_BP:
			{
				for (FString& Filename : FoundFilenames)
				{
					FString CleanFilename = FPaths::GetCleanFilename(Filename);
					UE_LOG(LogTemp, Log, TEXT(" 文件名为  %s"), *CleanFilename);
					if (CleanFilename.StartsWith(TEXT("BP_")) && CleanFilename.EndsWith(TEXT(".uasset")))
					{
						XH_GET_UASSET_NAME("Blueprint'", "_C'")
					}
				}
			}
				break;
			case XhLoadPakSource::Prefix_T:
			{
				for (FString& Filename : FoundFilenames)
				{
					FString CleanFilename = FPaths::GetCleanFilename(Filename);
					UE_LOG(LogTemp, Log, TEXT(" 文件名为  %s"), *CleanFilename);
					if (CleanFilename.StartsWith(TEXT("T_")) && CleanFilename.EndsWith(TEXT(".uasset")))
					{
						XH_GET_UASSET_NAME("Texture2D'", "'")
					}
				}
			}
				break;
			case XhLoadPakSource::Prefix_SM:
			{
				for (FString& Filename : FoundFilenames)
				{
					FString CleanFilename = FPaths::GetCleanFilename(Filename);
					UE_LOG(LogTemp, Log, TEXT(" 文件名为  %s"), *CleanFilename);
					if (CleanFilename.StartsWith(TEXT("SM_")) && CleanFilename.EndsWith(TEXT(".uasset")))
					{
						XH_GET_UASSET_NAME("StaticMesh'", "'")
					}
				}
			}
				break;
			case XhLoadPakSource::All:
			{
				for (FString& Filename : FoundFilenames)
				{
					OutResults.AddUnique(Filename);
				}
			}
				break;
			default:
				break;
			}
		}
	}
	//设置回原来的读取方式，不然包内的资源可能访问不了
	FPlatformFileManager::Get().SetPlatformFile(*OldPlatform);
}
#undef XH_GET_UASSET_NAME
void AXhLoadPak::XhLoadStreamLevels(const TArray<FString>& LevelNames, bool bIsLoad /*= true*/)
{
	if (!LevelNames.Num())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!bIsLoadingLevel)//没有加载
	{
		ToltalLoadLevels.AddUnique(XhLevelParam(LevelNames, bIsLoad));
		bIsLoadingLevel = true;
	}
	else if (ToltalLoadLevels[0].LevelNamesParam != LevelNames)//有其他加载
	{
		UE_LOG(LogTemp, Log, TEXT("已有其他关卡事件正在执行，已经排入队列中……"));
		ToltalLoadLevels.AddUnique(XhLevelParam(LevelNames, bIsLoad));
		return;
	}
	if (ToltalLoadLevels[0].CurrentIndex >= ToltalLoadLevels[0].LevelNamesParam.Num())
	{
		bIsLoadingLevel = false;
		FString LogStr = ToltalLoadLevels[0].ToString();
		UE_LOG(LogTemp, Log, TEXT("完成事件：[%s]"), *LogStr);
		ToltalLoadLevels.RemoveAt(0);
		if (ToltalLoadLevels.IsValidIndex(0))
		{
			LogStr = ToltalLoadLevels[0].ToString();
			UE_LOG(LogTemp, Log, TEXT("正准备执行事件：[%s]"), *LogStr);
			XhLoadStreamLevels(ToltalLoadLevels[0].LevelNamesParam, ToltalLoadLevels[0].bIsLoadParam);
		}
		return;
	}
	
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.UUID = FMath::Rand();
	LatentInfo.Linkage = 0;
	FString LevelName = ToltalLoadLevels[0].LevelNamesParam[ToltalLoadLevels[0].CurrentIndex++];
	int32 Pos = LevelName.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	LevelName = LevelName.RightChop(Pos);

	//UGameplayStatics::LoadStreamLevel(World, FName(*LevelName), true, false, LatentInfo);
	//UGameplayStatics::OpenLevel(World, FName(*LevelName));

	/* LoadOrUnloadStreamLevel 开始    直接在LoadStreamLevel中拿过来的，为了加载和卸载方便*/
	FLatentActionManager& LatentManager = World->GetLatentActionManager();
	if (LatentManager.FindExistingAction<FStreamLevelAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
	{
		FStreamLevelAction* NewAction = new FStreamLevelAction(bIsLoad, FName(*LevelName), bIsLoad, false, LatentInfo, World);
		FString LogString = LevelName + TEXT("已被") + (bIsLoad ? TEXT("加") : TEXT("卸")) + TEXT("载");
		UE_LOG(LogTemp, Log, TEXT("%s"), *LogString);
		LatentManager.AddNewAction(this, LatentInfo.UUID, NewAction);
		// 添加自定义的异步操作
		LatentManager.AddNewAction(
			this,
			LatentInfo.UUID,
			new FLevelLoadedLatentAction(
				[this, LevelNames, bIsLoad]()
				{
					this->XhLoadStreamLevels(LevelNames, bIsLoad);
				},
				TEXT("LoadLevelPost")
			)
		);
	}
	/* LoadOrUnloadStreamLevel 结束*/
}

FString AXhLoadPak::CreateStreamInstance(UWorld* World, const FString& LongPackageName)
{
	const FString ShortPackageName = FPackageName::GetShortName(LongPackageName);
	const FString PackagePath = FPackageName::GetLongPackagePath(LongPackageName);
	FString UniqueLevelPackageName = PackagePath + TEXT("/") + World->StreamingLevelsPrefix + ShortPackageName;
	// Setup streaming level object that will load specified map
	UClass* StreamingClass = ULevelStreamingDynamic::StaticClass();
	ULevelStreamingDynamic* StreamingLevel = NewObject<ULevelStreamingDynamic>(World, StreamingClass, NAME_None, RF_Transient, NULL);

	StreamingLevel->SetWorldAssetByPackageName(FName(*UniqueLevelPackageName));
	StreamingLevel->LevelColor = FColor::MakeRandomColor();

	StreamingLevel->LevelTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector);
	// Map to Load
	StreamingLevel->PackageNameToLoad = FName(*LongPackageName);
	StreamingLevel->SetShouldBeLoaded(false);
	StreamingLevel->SetShouldBeVisible(false);
	StreamingLevel->bShouldBlockOnLoad = false;
	World->AddUniqueStreamingLevel(StreamingLevel);
	return UniqueLevelPackageName;
}


// Called when the game starts or when spawned
void AXhLoadPak::BeginPlay()
{
	Super::BeginPlay();

	/*TArray<FString> LevelNames;
	FString LocPakPath;
	FFileHelper::LoadFileToString(LocPakPath, *(FPaths::ProjectContentDir() + "xh.txt"));
	FString PathDir = FPaths::ProjectContentDir() + LocPakPath;
	LoadLevelPak(LevelNames, *PathDir,XhLoadPakDir::Content,XhLoadPakSource::Prefix_BP);
	ProcessStreamLevels(LevelNames, true);*/
}

// Called every frame
void AXhLoadPak::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

