// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Engine/TextureRenderTarget2D.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "CesiumGeoreference.h"
#include "FoliageHISM.h"

#include "FoliageCaptureActor.generated.h"

/**
 * @brief Used to store the reprojected points gathered from the RT.
 */
USTRUCT(BlueprintType)
struct FFoliageTransforms
{
	GENERATED_BODY()
	TMap<UFoliageHISM*, TArray<FTransform>> HISMTransformMap;
};

/**
 * @brief Array of foliage type transforms gathered from the RT extraction task.
 */
USTRUCT()
struct FFoliageTransformsTypeMap
{
	GENERATED_BODY()
	TArray<FFoliageTransforms> FoliageTypes;
};

/**
 * @brief Foliage geometry container
 */
USTRUCT(BlueprintType)
struct FFoliageGeometryType
{
	GENERATED_BODY()

	/* Placement */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Placement")
	float Density = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Placement")
	bool bRandomYaw = false;

	UPROPERTY(EditAnywhere, Category = "Placement")
	FFloatInterval ZOffset = FFloatInterval(0.0, 0.0);

	UPROPERTY(EditAnywhere, Category = "Placement")
	FFloatInterval Scale = FFloatInterval(1.0, 1.0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Placement")
	bool bAlignToNormal = false;

	/* Mesh Settings */

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
	UStaticMesh* Mesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
	bool bCollidesWithWorld = true;

	UPROPERTY(EditAnywhere, Category = "Placement")
	FFloatInterval CullingDistances = FFloatInterval(4096, 32768);

	/* Expensive */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
	bool bAffectsDistanceFieldLighting = false;

	friend uint32 GetTypeHash(const FFoliageGeometryType& A)
	{
		return GetTypeHash(A.Density) + GetTypeHash(A.bRandomYaw) + GetTypeHash(A.ZOffset) + GetTypeHash(A.Scale) +
			GetTypeHash(A.Mesh) + GetTypeHash(A.bCollidesWithWorld) +
			GetTypeHash(A.bAffectsDistanceFieldLighting);
	}

	friend bool operator==(const FFoliageGeometryType& A, const FFoliageGeometryType& B)
	{
		return A.Density == B.Density && A.Mesh == B.Mesh && A.bCollidesWithWorld == B.bCollidesWithWorld && A.
			bAffectsDistanceFieldLighting == B.bAffectsDistanceFieldLighting
			&& A.Scale.Max == B.Scale.Max && A.Scale.Min == B.Scale.Min && A.bRandomYaw == B.bRandomYaw &&
			A.ZOffset.Min == B.ZOffset.Min && A.ZOffset.Max == B.ZOffset.Max;
	}
};

/**
 * @brief Container for a foliage type
 */
USTRUCT(BlueprintType)
struct FFoliageClassificationType
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Type;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor ColourClassification;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FFoliageGeometryType> FoliageTypes;

	/**
	 * @brief If enabled, a line trace will be casted downwards from each point to determine surface normals.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAlignToSurfaceWithRaycast = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 PooledHISMsToCreatePerFoliageType = 4;
};

// Called after points have been gathered and reprojected from the classification RT.
DECLARE_DELEGATE_OneParam(FOnFoliageTransformsGenerated, FFoliageTransformsTypeMap);

// This is called after pixels have been extracted from input RTs
DECLARE_DELEGATE_OneParam(FOnRenderTargetRead, bool);

UCLASS()
class AIDEN_GEO_TUTORIAL_API AFoliageCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AFoliageCaptureActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage Spawner")
	TArray<FFoliageClassificationType> FoliageTypes;

	UPROPERTY()
	ACesiumGeoreference* Georeference;

	/**
	 * @brief Elevation (in metres) of the scene capture component that's placed above the player.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage Spawner")
	float CaptureElevation = 1024.f;

	/**
	 * @brief Orthographic width of our scene capture components
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage Spawner")
	float CaptureWidth = 131072.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage Spawner")
	int32 UpdateFoliageAfterNumFrames = 2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage Spawner")
	int32 MaxComponentsToUpdatePerFrame = 1;

	/**
	 * @brief Coverage grid.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage Spawner")
	FIntVector GridSize = FIntVector(0, 0, 0);

	/**
	* @brief Average geographic width in degrees.
	*/
	double CaptureWidthInDegrees = 0.01;

public:
	/**
	 * @brief Build foliage transforms according to classification types.
	 * @param FoliageDistributionMap Render Target containing classifications. 
	 * @param NormalAndDepthMap Render Target with normals in the RGB channel and *normalized* depth in Alpha.
	 * @param RTWorldBounds UE world extents of the render targets.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage Spawner")
	void BuildFoliageTransforms(UTextureRenderTarget2D* FoliageDistributionMap,
	                            UTextureRenderTarget2D* NormalAndDepthMap, FBox RTWorldBounds);

	/**
	 * @brief Create required HISM components, removing if outdated
	 */
	void ResetAndCreateHISMComponents();

	/**
	 * @brief Implementable BP event, called when the player moves outside of the capture boundaries.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Foliage Spawner")
	void OnUpdate(const FVector& NewLocation);

	/**
	 * @brief Is the foliage currently building?
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Foliage Spawner")
	bool IsBuilding() const;

protected:
	/**
	 * @brief Attempt to correct normals and elevation by raycasting
	 */
	void CorrectFoliageTransform(const FVector& InEngineCoordinates, const FMatrix& InEastNorthUp,
	                             FVector& OutCorrectedPosition, FVector& OutSurfaceNormals, bool& bSuccess) const;

	/**
	 * @brief For each static mesh, we also want to have multiple HISM components to reduce
	 * hitches when updating instances.
	 */
	TMap<FFoliageGeometryType, TArray<UFoliageHISM*>> HISMFoliageMap;

	/**
	 * @brief The scene depth value is multiplied by a small value so it remains within the range of 0.0 to 1.0.
	 * In this function it is projected back to it's (approximated) original value and then inverted.
	 * @param Value Depth value
	 * @return Height in metres.
	 */
	double GetHeightFromDepth(const double& Value) const;


	/**
	 * @brief Converts pixel coordinates back to geographic coordinates.
	 */
	glm::dvec3 PixelToGeographicLocation(const double& X, const double& Y, const double& Altitude,
	                                     UTextureRenderTarget2D* RT, const glm::dvec4& GeographicExtents) const;
	/**
	 * @brief Converts geographic coordinates to pixel coordinates.
	 */
	FIntPoint GeographicToPixelLocation(const double& Longitude, const double& Latitude, UTextureRenderTarget2D* RT,
	                                    const glm::dvec4& GeographicExtents) const;

	/**
	 * @brief Modified version of ReadRenderColorPixels
	 */
	void ReadLinearColorPixelsAsync(
		FOnRenderTargetRead OnRenderTargetRead,
		TArray<FTextureRenderTargetResource*> RTs,
		TArray<TArray<FLinearColor>*> OutImageData,
		FReadSurfaceDataFlags InFlags = FReadSurfaceDataFlags(
			RCM_MinMax,
			CubeFace_MAX),
		FIntRect InRect = FIntRect(0, 0, 0, 0),
		ENamedThreads::Type ExitThread = ENamedThreads::AnyBackgroundThreadNormalTask);

	/**
	 * @brief Don't run tick update if true.
	 */
	bool bIsBuilding = false;

	/**
	 * @brief Number of frames that have passed after updating foliage.
	 */
	int32 Ticks = 0;

	static glm::dvec3 VectorToDVector(const FVector& InVector);

	int32 GetInstanceCount(float Delta);
};

inline int32 AFoliageCaptureActor::GetInstanceCount(float Delta)
{
	int32 Count = 0;
	bool bAnyHISMValid = false;
	for (auto T : HISMFoliageMap)
	{
		for (auto V : T.Value)
		{
			if (IsValid(V))
			{
				Count += V->GetInstanceCount();
				bAnyHISMValid = true;
			}
		}
	}

	return bAnyHISMValid ? Count : -1;
}