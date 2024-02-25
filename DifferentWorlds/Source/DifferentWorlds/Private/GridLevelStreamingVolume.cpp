// Fill out your copyright notice in the Description page of Project Settings.


#include "GridLevelStreamingVolume.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

AGridLevelStreamingVolume::AGridLevelStreamingVolume()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AGridLevelStreamingVolume::BeginPlay()
{
	Super::BeginPlay();

	currentCell[0] = currentCell[1] = -1;
	prevCell[0] = prevCell[1] = -1;
	float cellSizeX = Extents.X / ColumnsCount;
	float cellSizeY = Extents.Y / RowsCount;

	cellSize = FVector(cellSizeX, cellSizeY, Extents.Z);
	cells = new TArray<TArray<FVector>*>;
	cells->Init(nullptr, RowsCount);
	for (size_t i = 0; i < RowsCount; i++)
	{
		(*cells)[i] = new TArray<FVector>;
		(*cells)[i]->Init(FVector::ZeroVector, ColumnsCount);
		for (size_t j = 0; j < ColumnsCount; j++)
		{
			float x = cellSizeX * j + Origin.X;
			float y = cellSizeY * i + Origin.Y;
			(*(*cells)[i])[j] = FVector(x, y, Origin.Z);
		}
	}
}

void AGridLevelStreamingVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (currentCell.X == -1 || currentCell.Y == -1)
	{
		FindPlayerOverlappingCell();
	}
	else
	{
		if (!CheckIfPlayerOverlapping((*(*cells)[currentCell.X])[currentCell.Y]))
		{
			if (!CheckIfPlayerOverlappingInNeighbours())
			{
				// Rare case where the player is suddenly ported to farther cell
				currentCell = FInt32Vector2(-1, -1);
			}
		}
	}
}

bool AGridLevelStreamingVolume::CheckIfPlayerOverlapping(FVector cellOrigin)
{
	auto player = UGameplayStatics::GetPlayerPawn(this, 0);
	auto playerPosition = player->GetActorLocation();
	float cellStartX = cellOrigin.X - (cellSize.X / 2.0f);
	float cellEndX = cellOrigin.X + (cellSize.X / 2.0f);
	float cellStartY = cellOrigin.Y - (cellSize.Y / 2.0f);
	float cellEndY = cellOrigin.Y + (cellSize.Y / 2.0f);
	if (cellStartX < playerPosition.X && playerPosition.X <= cellEndX &&
		cellStartY < playerPosition.Y && playerPosition.Y <= cellEndY)
	{
		return true;
	}
	return false;
}

bool AGridLevelStreamingVolume::CheckIfPlayerOverlappingInNeighbours()
{
	// Check neighbouring cells
	for (int i = currentCell.X - 1; i <= currentCell.X + 1; i++)
	{
		if (i < 0 || i >= RowsCount) continue;
		for (int j = currentCell.Y - 1; j <= currentCell.Y + 1; j++)
		{
			if (j < 0 || j >= ColumnsCount || (i == currentCell.X && j == currentCell.Y)) continue;
			if (CheckIfPlayerOverlapping((*(*cells)[i])[j]))
			{
				prevCell = currentCell;
				currentCell = FInt32Vector2(i,j);
				UKismetSystemLibrary::PrintString(this, "Overlapping cell : " + FString::FromInt(i) + "," + FString::FromInt(j));
				
				ReloadAllLevels(false);
				return true;
			}
		}
	}

	return false;
}

void AGridLevelStreamingVolume::FindPlayerOverlappingCell()
{
	for (size_t i = 0; i < RowsCount; i++)
	{
		for (size_t j = 0; j < ColumnsCount; j++)
		{
			UKismetSystemLibrary::DrawDebugBox(this, (*(*cells)[i])[j], FVector::OneVector, FLinearColor::Red);
			if (CheckIfPlayerOverlapping((*(*cells)[i])[j]))
			{
				prevCell = currentCell;
				currentCell = FInt32Vector2(i, j);

				ReloadAllLevels(true);
				UKismetSystemLibrary::PrintString(this, "Overlapping cell : " + FString::FromInt(i) + "," + FString::FromInt(j));

				return;
			}
		}
	}
}

void AGridLevelStreamingVolume::ReloadAllLevels(bool fullReload)
{
	if (fullReload)
	{
		UnloadAllOldLevels();
		LoadAllNewLevels();
	}
	else
	{
		if (prevCell.X != currentCell.X)
		{
			int rowToUnload = prevCell.X + (NeighbourRange / 2) * (prevCell.X - currentCell.X);
			int rowToLoad = currentCell.X - (NeighbourRange / 2) * (prevCell.X - currentCell.X);
			UnloadRowLevels(rowToUnload, prevCell.Y - (NeighbourRange / 2));
			LoadRowLevels(rowToLoad, currentCell.Y - (NeighbourRange / 2));

		}
		if (prevCell.Y != currentCell.Y)
		{
			int colToUnload = prevCell.Y + (NeighbourRange / 2) * (prevCell.Y - currentCell.Y);
			int colToLoad = currentCell.Y - (NeighbourRange / 2) * (prevCell.Y - currentCell.Y);
			UnloadColumnLevels(colToUnload, prevCell.X - (NeighbourRange / 2));
			LoadColumnLevels(colToLoad, currentCell.X - (NeighbourRange / 2));
		}
	}
}

void AGridLevelStreamingVolume::UnloadRowLevels(int row, int startCol)
{
	for (int i = startCol; i < startCol + NeighbourRange; i++)
	{
		UnloadLevelAt(row, i);
	}
}

void AGridLevelStreamingVolume::UnloadColumnLevels(int col, int startRow)
{
	for (int i = startRow; i < startRow + NeighbourRange; i++)
	{
		UnloadLevelAt(i, col);
	}
}

void AGridLevelStreamingVolume::UnloadAllOldLevels()
{
	for (auto level : loadedLevels)
	{
		UnloadLevelAt(level.Key.X, level.Key.Y);
	}
}

void AGridLevelStreamingVolume::LoadAllNewLevels()
{
	for (int i = currentCell[0]-(NeighbourRange/2); i <= currentCell[0]+ (NeighbourRange / 2); i++)
	{
		LoadRowLevels(i, currentCell[1] - (NeighbourRange / 2));
	}
}


void AGridLevelStreamingVolume::LoadLevelAt(int row, int col)
{
	if (row < 0 || col < 0 || row >= RowsCount || col >= ColumnsCount)
		return;
	FUint32Vector2 key = FUint32Vector2(row, col);
	if (loadedLevels.Contains(key))
		return;
	auto position = (*(*cells)[row])[col];

	bool isSuccess = false;
	ULevelStreamingDynamic::FLoadLevelInstanceParams loadLevelParams(GetWorld(), "/Game/Levels/Forest", FTransform(FRotator::ZeroRotator, position));
	loadLevelParams.bInitiallyVisible = true;
	auto level = ULevelStreamingDynamic::LoadLevelInstance(loadLevelParams, isSuccess);
	loadedLevels.Add(key, level);
}


void AGridLevelStreamingVolume::UnloadLevelAt(int row, int col)
{
	if (row < 0 || col < 0 || row >= RowsCount || col >= ColumnsCount)
		return;
	FUint32Vector2 key = FUint32Vector2(row, col);
	if (loadedLevels.Contains(key))
	{
		auto levelToUnload = loadedLevels[key];
		levelToUnload->SetShouldBeLoaded(false);
		loadedLevels.Remove(key);
	}
}

void AGridLevelStreamingVolume::LoadRowLevels(int row, int startCol)
{
	for (int i = startCol; i < startCol + NeighbourRange; i++)
	{
		LoadLevelAt(row, i);
	}
}

void AGridLevelStreamingVolume::LoadColumnLevels(int col, int startRow)
{
	for (int i = startRow; i < startRow + NeighbourRange; i++)
	{
		LoadLevelAt(i, col);
	}
}