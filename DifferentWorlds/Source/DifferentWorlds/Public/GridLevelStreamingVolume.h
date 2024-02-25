// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelStreamingDynamic.h"
#include "GridLevelStreamingVolume.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class DIFFERENTWORLDS_API AGridLevelStreamingVolume : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	int ColumnsCount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	int RowsCount;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	int NeighbourRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	FVector Extents;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	FVector Origin;
	AGridLevelStreamingVolume();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	TArray<TArray<FVector>*>* cells;
	TMap<FUint32Vector2, ULevelStreamingDynamic*> loadedLevels;
	FVector cellSize;
	FInt32Vector2 currentCell;
	FInt32Vector2 prevCell;
	bool CheckIfPlayerOverlapping(FVector cellOrigin);
	bool CheckIfPlayerOverlappingInNeighbours();
	void FindPlayerOverlappingCell();
	void ReloadAllLevels(bool fullReload);
	void UnloadRowLevels(int row, int startCol);
	void UnloadColumnLevels(int col, int startRow);
	void UnloadAllOldLevels();
	void LoadAllNewLevels();
	void LoadLevelAt(int row, int col);
	void UnloadLevelAt(int row, int col);
	void LoadRowLevels(int row, int startCol);
	void LoadColumnLevels(int col, int startRow);
};
