#include "Game/BaamDataSubsystem.h"

#include "Game/BaamDataSettings.h"
#include "Game/BaamCardType.h"
#include "UObject/ConstructorHelpers.h"

const FString UBaamDataSubsystem::CharacterTablePath = TEXT("/Game/GameSystem/Data/DT_BaamCharacterRow");

void UBaamDataSubsystem::BuildDeck(TArray<FBaamCardInstance>& OutDeck) const
{
	OutDeck.Reset();
	
	TArray<FName> RowNames = CardTable->GetRowNames();

	int32 NextInstanceId = 1;   // 0 은 "미설정" 과 구분하기 위해 쓰지 않는다
	int32 NumKinds = 0;	//	카드 종류 ( 로그 출력용 )

	for (const FName& RowName : RowNames)
	{
		const FBaamCardRow* Row = CardTable->FindRow<FBaamCardRow>(RowName, TEXT("BuildDeck"));
		if (!Row || Row->QuantityOfCard <= 0)
		{
			continue;   // QuantityOfCard 0 = 의도적 제외 (아직 미구현 카드 등)
		}

		for (int32 i = 0; i < Row->QuantityOfCard; ++i)
		{
			FBaamCardInstance Card;
			Card.InstanceId = NextInstanceId++;
			Card.CardId = RowName;
			OutDeck.Add(Card);
		}
		++NumKinds;
	}

	UE_LOG(LogTemp, Log, TEXT("BuildDeck:%d 종류 →%d 장"), NumKinds, OutDeck.Num());
}

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

void UBaamDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const UBaamDataSettings* Settings = GetDefault<UBaamDataSettings>();
	CardTable = Settings->CardTable.LoadSynchronous();
	check(CardTable != nullptr);	//	BuildDeck: CardTable 이 없습니다 (프로젝트 설정 확인).
}

const FBaamCardRow* UBaamDataSubsystem::GetCardRow(FName CardID) const
{
	const FBaamCardRow* Row = CardTable->FindRow<FBaamCardRow>(CardID, TEXT("GetCardRow"));
	return Row;
}

const FBaamCardRow* UBaamDataSubsystem::GetCardRow(FGameplayTag CardTag) const
{
	TArray<FName> RowNames = CardTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		const FBaamCardRow* Row = CardTable->FindRow<FBaamCardRow>(RowName, TEXT("GetCardRow"));
		if (!Row || Row->CardIdTag == CardTag)
		{
			return Row;
		}
	}
	return nullptr;
}
