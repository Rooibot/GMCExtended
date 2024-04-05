#pragma once

#include "CoreMinimal.h"
#include "GMCExtendedAnimation.h"
#include "StructUtilsTypes.h"
#include "UObject/Object.h"
#include "GMCE_DeltaArray.generated.h"

enum class EGMCE_FastArrayOperation : uint8
{
	Add,
	Update
};

USTRUCT()
struct FGMCE_DeltaArrayItem
{
	GENERATED_BODY()

	// A unique identifier for this specific item without our fast array serializer.
	UPROPERTY(NotReplicated)
	int32 ItemReplicationID { INDEX_NONE };

	// A key which we increment every time the item is marked dirty.
	UPROPERTY(NotReplicated)
	int32 ItemReplicationKey { INDEX_NONE };

	// Replication key of our parent array the last time we were sent.
	UPROPERTY(NotReplicated)
	int32 LastArrayReplicationKey { INDEX_NONE };

	void MarkArrayItemDirty()
	{
		// If we haven't yet been assigned a replication ID, we are by definition dirty
		// and don't need to churn our replication key; we'll be replicated once we get
		// mapped.
		if (ItemReplicationID != INDEX_NONE) ItemReplicationKey++;
	}

	FString ToItemString() const
	{
		return FString::Printf(TEXT("ID %d rep %d"), ItemReplicationID, ItemReplicationKey);
	}

	FGMCE_DeltaArrayItem& operator=(const FGMCE_DeltaArrayItem& Other)
	{
		// When setting a new value, we must also reset our replication key data.
		if (&Other != this)
		{
			UE_LOG(LogGMCExAnimation, Log, TEXT("Resetting due to reassignment"))
			ItemReplicationID = INDEX_NONE;
			ItemReplicationKey = INDEX_NONE;
			LastArrayReplicationKey = INDEX_NONE;
		}
		return *this;
	}
};



USTRUCT()
struct FGMCE_DeltaArraySerializer
{
	GENERATED_BODY()

	// Key is a replication ID, value is the replication key as of the last time we replicated.
	UPROPERTY()
	TMap<int32, int32> ReplicationLastKeyMap {};

	UPROPERTY()
	int32 NextReplicationIdCounter { 0 };

	UPROPERTY()
	int32 LastArrayReplicationKey { INDEX_NONE };

	UPROPERTY()
	int32 ArrayReplicationKey { 0 };

	// Marks the item dirty. 
	void MarkItemDirty(FGMCE_DeltaArrayItem& Item)
	{
		if (Item.ItemReplicationID == INDEX_NONE)
		{
			// Assign an ID.
			Item.ItemReplicationID = NextReplicationIdCounter++;
			UE_LOG(LogGMCExAnimation, Log, TEXT("Assigning new ID: %s"), *Item.ToItemString())
		}

		Item.MarkArrayItemDirty();
	}

	void IncrementArrayReplicationKey()
	{
		ArrayReplicationKey++;
	}

	void MarkArrayDirty()
	{
		UE_LOG(LogGMCExAnimation, Log, TEXT("Marking array dirty"))
		// Clear all our mappings.
		ReplicationLastKeyMap.Reset();
		IncrementArrayReplicationKey();
	}

	template<typename ItemType, typename StructType>
	static bool SerializeDeltaArray(StructType& ContainerStruct, TArray<ItemType>& SourceArray, FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	template<typename ItemType, typename StructType>
	static void CollectChangedItems(StructType& ContainerStruct, TArray<ItemType>& SourceArray, TMap<int32, EGMCE_FastArrayOperation>& OutputMap, bool bResetArray);

	inline static uint8 Version = 1;

};

template <typename ItemType, typename StructType>
bool FGMCE_DeltaArraySerializer::SerializeDeltaArray(StructType& ContainerStruct, TArray<ItemType>& SourceArray,
	FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!TIsDerivedFrom<ItemType, FGMCE_DeltaArrayItem>::IsDerived)
	{
		return false;
	}
	
	if (Ar.IsSaving())
	{
		bool bResetArray = (ContainerStruct.LastArrayReplicationKey != ContainerStruct.ArrayReplicationKey);

		TMap<int32, EGMCE_FastArrayOperation> ChangedItemIDs;
		FGMCE_DeltaArraySerializer::CollectChangedItems<ItemType, StructType>(ContainerStruct, SourceArray, ChangedItemIDs, bResetArray);

		uint32 VersionNumber = Version;
		uint32 ChangedNum = ChangedItemIDs.Num();
		uint32 ArrayKey = ContainerStruct.ArrayReplicationKey;
		
		Ar.SerializeIntPacked(VersionNumber);
		Ar.SerializeBits(&bResetArray, 1);
		Ar.SerializeIntPacked(ArrayKey);
		Ar.SerializeIntPacked(ChangedNum);

		for (uint32 Idx = 0; Idx < static_cast<uint32>(SourceArray.Num()); Idx++)
		{
			auto& Item = SourceArray[Idx];
				
			if (ChangedItemIDs.Contains(Item.ItemReplicationID))
			{
				Item.LastArrayReplicationKey = ContainerStruct.ArrayReplicationKey;
				uint32 ReplicationID = Item.ItemReplicationID;
				uint32 ReplicationKey = Item.ItemReplicationKey;

				UScriptStruct* ScriptStruct = UE::StructUtils::GetAsUStruct<ItemType>();
				bool bIsAdded = ChangedItemIDs[Item.ItemReplicationID] == EGMCE_FastArrayOperation::Add;
				
				Ar.SerializeIntPacked(Idx);
				Ar.SerializeIntPacked(ReplicationID);
				Ar.SerializeIntPacked(ReplicationKey);
				Ar.SerializeBits(&bIsAdded, 1);
				ScriptStruct->SerializeItem(Ar, &Item, nullptr);

				ContainerStruct.ReplicationLastKeyMap.Add(Item.ItemReplicationID, Item.ItemReplicationKey);
			}
		}

		ContainerStruct.LastArrayReplicationKey = ContainerStruct.ArrayReplicationKey;
		bOutSuccess = true;
		return true;
	}
	else if (Ar.IsLoading())
	{
		uint32 LoadVersion;
		bool bShouldReset;
		uint32 ArrayKey;
		uint32 NumRecords;
		
		Ar.SerializeIntPacked(LoadVersion);

		// Handle any version-specific nonsense here.

		Ar.SerializeBits(&bShouldReset, 1);
		Ar.SerializeIntPacked(ArrayKey);
		Ar.SerializeIntPacked(NumRecords);
		if (bShouldReset)
		{
			SourceArray.Empty();
			SourceArray.AddUninitialized(NumRecords);
		}
		
		ContainerStruct.ArrayReplicationKey = ArrayKey;
		ContainerStruct.LastArrayReplicationKey = ArrayKey;

		for (uint32 ReadIdx = 0; ReadIdx < NumRecords; ReadIdx++)
		{
			uint32 ArrayPos, ReplicationID, ReplicationKey;
			bool bIsAdded;
			ItemType Item;

			UScriptStruct* ScriptStruct = UE::StructUtils::GetAsUStruct<ItemType>();

			Ar.SerializeIntPacked(ArrayPos);
			Ar.SerializeIntPacked(ReplicationID);
			Ar.SerializeIntPacked(ReplicationKey);
			Ar.SerializeBits(&bIsAdded, 1);
			ScriptStruct->SerializeItem(Ar, &Item, nullptr);

			Item.ItemReplicationID = ReplicationID;
			Item.ItemReplicationKey = ReplicationKey;
			Item.LastArrayReplicationKey = ArrayKey;
			
			if (bIsAdded)
			{
				if (!SourceArray.IsValidIndex(ArrayPos))
				{
					SourceArray.AddUninitialized(ArrayPos - SourceArray.Num());
				}
				SourceArray[ArrayPos] = Item;
			}
			else
			{
				ensureAlways(SourceArray.IsValidIndex(ArrayPos));
				ItemType& CurrentItem = SourceArray[ArrayPos];

				ensureAlways(Item.ItemReplicationID == CurrentItem.ItemReplicationID);

				if (CurrentItem.ItemReplicationKey != Item.ItemReplicationKey)
				{
					SourceArray[ArrayPos] = Item;
				}
			}

			ContainerStruct.ReplicationLastKeyMap.Add(Item.ItemReplicationID, Item.ItemReplicationKey);
			Item.LastArrayReplicationKey = ContainerStruct.ArrayReplicationKey;
		}
		
		bOutSuccess = true;
		return true;
	}

	return false;
}

template <typename ItemType, typename StructType>
void FGMCE_DeltaArraySerializer::CollectChangedItems(StructType& ContainerStruct, TArray<ItemType>& SourceArray,
	TMap<int32, EGMCE_FastArrayOperation>& OutputMap, bool bResetArray)
{
	OutputMap.Reset();

	for (int32 Idx = 0; Idx < SourceArray.Num(); Idx++)
	{
		ItemType& Item = SourceArray[Idx];
		
		if (Item.ItemReplicationID == INDEX_NONE)
		{
			ContainerStruct.MarkItemDirty(Item);
		}
		
		if (bResetArray || !ContainerStruct.ReplicationLastKeyMap.Contains(Item.ItemReplicationID) ||
			ContainerStruct.ReplicationLastKeyMap[Item.ItemReplicationID] != Item.ItemReplicationKey)
		{
			// We need to include this.
			if (bResetArray || !ContainerStruct.ReplicationLastKeyMap.Contains(Item.ItemReplicationID))
			{
				OutputMap.Add(Item.ItemReplicationID, EGMCE_FastArrayOperation::Add);
			}
			else
			{
				OutputMap.Add(Item.ItemReplicationID, EGMCE_FastArrayOperation::Update);
			}
		}		
	}
}
