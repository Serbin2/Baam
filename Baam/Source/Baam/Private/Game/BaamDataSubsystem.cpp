#include "Game/BaamDataSubsystem.h"
#include "UObject/ConstructorHelpers.h"

const FString UBaamDataSubsystem::CharacterTablePath = TEXT("/Game/GameSystem/Data/DT_BaamCharacterRow");
const FString UBaamDataSubsystem::CardProbabilityTablePath = TEXT("/Game/GameSystem/Data/DT_BaamCardProbability");

UBaamDataSubsystem::UBaamDataSubsystem()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> DT_Character(*CharacterTablePath);
	if (DT_Character.Succeeded())
	{
		CharacterTable = DT_Character.Object;
	}

	static ConstructorHelpers::FObjectFinder<UDataTable> DT_CardProb(*CardProbabilityTablePath);
	if (DT_CardProb.Succeeded())
	{
		CardProbabilityTable = DT_CardProb.Object;
	}
}

const FBaamCharacterRow* UBaamDataSubsystem::GetCharacterRow(FGameplayTag CharacterTag) const
{
	if (!CharacterTable || !CharacterTag.IsValid())
	{
		return nullptr;
	}

	const FName RowName = CharacterTag.GetTagName();
	return CharacterTable->FindRow<FBaamCharacterRow>(RowName, TEXT("BaamCharacterRow"));
}

void UBaamDataSubsystem::GetAllCardProbabilities(TArray<FBaamCardProbabilityRow>& OutRows) const
{
	OutRows.Reset();
	if (!CardProbabilityTable)
	{
		return;
	}

	TArray<FBaamCardProbabilityRow*> Rows;
	CardProbabilityTable->GetAllRows<FBaamCardProbabilityRow>(TEXT("BaamCardProbability"), Rows);

	OutRows.Reserve(Rows.Num());
	for (const FBaamCardProbabilityRow* Row : Rows)
	{
		if (Row)
		{
			OutRows.Add(*Row);
		}
	}
}
