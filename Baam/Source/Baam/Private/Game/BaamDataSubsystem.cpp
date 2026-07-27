#include "Game/BaamDataSubsystem.h"
#include "UObject/ConstructorHelpers.h"

const FString UBaamDataSubsystem::CharacterTablePath = TEXT("/Game/GameSystem/Data/DT_BaamCharacterRow");

UBaamDataSubsystem::UBaamDataSubsystem()
{
	static ConstructorHelpers::FObjectFinder<UDataTable> DT_Character(*CharacterTablePath);
	if (DT_Character.Succeeded())
	{
		CharacterTable = DT_Character.Object;
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
