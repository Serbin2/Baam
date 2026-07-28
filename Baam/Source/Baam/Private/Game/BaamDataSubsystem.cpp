#include "Game/BaamDataSubsystem.h"

#include "Game/BaamCardLog.h"
#include "Game/BaamDataSettings.h"
#include "Game/BaamCardType.h"
#include "UObject/ConstructorHelpers.h"

const FString UBaamDataSubsystem::CharacterTablePath = TEXT("/Game/GameSystem/Data/DT_BaamCharacterRow");
const FString UBaamDataSubsystem::CardProbabilityTablePath = TEXT("/Game/GameSystem/Data/DT_BaamCardProbability");

void UBaamDataSubsystem::BuildDeck(TArray<FBaamCardInstance>& OutDeck) const
{
	OutDeck.Reset();

	//	Initialize 의 check() 는 Shipping 에서 컴파일 제외된다. 실제 역참조 지점에서 한 번 더 막는다.
	if (!CardTable)
	{
		UE_LOG(LogBaamCard, Error,
			TEXT("BuildDeck: CardTable 이 없습니다. Project Settings > Game > Baam Data > CardTable 을 확인하세요."));
		return;
	}

	TArray<FName> RowNames = CardTable->GetRowNames();

	//	Row 순서를 명시적으로 고정한다. GetRowNames() 는 DT 내부 순서를 그대로 주므로,
	//	정렬하지 않으면 DT 에 카드 행을 하나 추가하는 것만으로 같은 시드가 다른 판을 만든다.
	//	FastLess() 가 아니라 Compare() 를 쓴다 — FastLess 는 FName 등록 순서에 의존해
	//	실행마다 순서가 달라질 수 있고, 재현성이 무너지면 카드 버그 추적이 불가능해진다.
	RowNames.Sort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });

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

	UE_LOG(LogBaamCard, Log, TEXT("BuildDeck:%d 종류 →%d 장"), NumKinds, OutDeck.Num());
}

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

TArray<FBaamCardInstance> UBaamDataSubsystem::GetDeck() const
{
	return Deck;
}

void UBaamDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const UBaamDataSettings* Settings = GetDefault<UBaamDataSettings>();
	CardTable = Settings->CardTable.LoadSynchronous();
	check(CardTable != nullptr);	//	BuildDeck: CardTable 이 없습니다 (프로젝트 설정 확인).

	//	check() 는 Shipping 에서 컴파일 제외되므로, 설정 누락 시 여기서 크래시하지 않고 덱 없이 계속한다.
	//	Baam_DumpDeck 이 "덱이 비어 있습니다" 로 원인을 알려준다.
	if (!CardTable)
	{
		UE_LOG(LogBaamCard, Error,
			TEXT("Initialize: CardTable 로드 실패 — 덱 없이 계속합니다. "
			     "Project Settings > Game > Baam Data > CardTable 을 확인하세요."));
		return;
	}

	BuildDeck(Deck);
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
		if (Row && Row->CardIdTag == CardTag)
		{
			return Row;
		}
	}
	return nullptr;
}
