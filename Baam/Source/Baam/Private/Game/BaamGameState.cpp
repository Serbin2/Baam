// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BaamGameState.h"

#include "Engine/GameInstance.h"
#include "Game/BaamCardLog.h"
#include "Game/BaamDataSubsystem.h"
#include "Game/BaamDiceComponent.h"
#include "Game/BaamGameplayTags.h"
#include "Net/UnrealNetwork.h"

ABaamGameState::ABaamGameState()
{
	//	확률 판정은 컴포넌트가 맡는다 (GameMode 가 아니라 여기 — 클라도 설정을 읽어야 하고
	//	시드 난수의 소유자가 GameState 이기 때문).
	Dice = CreateDefaultSubobject<UBaamDiceComponent>(TEXT("Dice"));
}

void ABaamGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABaamGameState, DeckCount);
	DOREPLIFETIME(ABaamGameState, DiscardCount);
	DOREPLIFETIME(ABaamGameState, DiscardTop);
	DOREPLIFETIME(ABaamGameState, PhaseTag);
}

void ABaamGameState::BeginPlay()
{
	Super::BeginPlay();

	//	덱은 서버만 만든다. 클라의 Deck 배열은 영원히 비어 있는 것이 정상
	if (HasAuthority())
	{
		InitializeDeck();
		SetPhaseTag(Bang::Phase::Lobby.GetTag());
	}
}

void ABaamGameState::SetPhaseTag(FGameplayTag InPhase)
{
	if (!HasAuthority() || PhaseTag == InPhase)
	{
		return;
	}

	PhaseTag = InPhase;
	UE_LOG(LogBaamCard, Log, TEXT("[GameState] 페이즈 → %s"), *InPhase.ToString());
}

void ABaamGameState::InitializeDeck(int32 Seed)
{
	if (!HasAuthority())
	{
		return;
	}
	
	//	시드 보존
	if (bInitialized)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[InitializeDeck] 이미 초기화됨 — 무시. 재시작하려면 bInitialized 를 먼저 되돌릴 것."));
		return;
	}

	const UGameInstance* GI = GetGameInstance();
	UBaamDataSubsystem* Subsystem = GI ? GI->GetSubsystem<UBaamDataSubsystem>() : nullptr;
	if (!Subsystem)
	{
		UE_LOG(LogBaamCard, Error, TEXT("[InitializeDeck] BaamDataSubsystem 없음 — 덱을 만들지 못했습니다."));
		return;
	}

	//	Seed 0 은 "자동" 을 뜻한다. 실제로 쓴 값을 반드시 로그에 남겨야 같은 판을 재현할 수 있다.
	int32 ActualSeed = Seed;
	if (ActualSeed == 0)
	{
		ActualSeed = FMath::Rand();
		if (ActualSeed == 0)
		{
			ActualSeed = 1;   //	0 은 "자동" 의미로 예약되어 있으므로 시드로 쓰지 않는다.
		}
	}
	RandomStream.Initialize(ActualSeed);

	//	판정 스트림도 같은 매치 시드에서 파생시킨다(재현 가능). 수열은 셔플과 분리된다.
	if (Dice)
	{
		Dice->InitializeStream(ActualSeed);
	}

	Subsystem->BuildDeck(Deck);
	Discard.Reset();
	DiscardTop = FBaamCardInstance();
	ShuffleDeck();
	RefreshPublicCounts();

	bInitialized = true;

	UE_LOG(LogBaamCard, Warning, TEXT("[InitializeDeck] 덱 생성 완료. %d 장, 사용 시드 : %d"), Deck.Num(), ActualSeed);
}

bool ABaamGameState::DrawFromDeck(FBaamCardInstance& OutCard)
{
	if (!HasAuthority())
	{
		return false;
	}
	
	if (Deck.IsEmpty())
	{
		//	덱 소진 → 버린 패를 섞어 되돌린다. DiscardTop 은 공개 상태로 남으므로 섞이지 않는다.
		ReshuffleDeck();

		if (Deck.IsEmpty())
		{
			//	되돌릴 카드도 없다. 정상적인 판에서는 나오지 않는다.
			UE_LOG(LogBaamCard, Warning, TEXT("[DrawFromDeck] 덱과 버린 패가 모두 비었습니다."));
			return false;
		}
	}

	//	RemoveAtSwap 을 쓰면 마지막 카드가 0 번으로 끌려와 셔플한 순서와 뽑는 순서가 달라진다.
	OutCard = Deck[0];
	Deck.RemoveAt(0);
	RefreshPublicCounts();
	return true;
}

void ABaamGameState::PushToDiscard(const FBaamCardInstance& Card)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (DiscardTop.IsValid())
	{
		Discard.Add(DiscardTop);
	}

	//	맨 위의 버림카드를 새로 들어온 카드로 변경
	DiscardTop = Card;
	RefreshPublicCounts();
}

void ABaamGameState::RefreshPublicCounts()
{
	DeckCount = Deck.Num();
	//	DiscardTop 은 Discard 배열 바깥에 따로 있으므로 더해 준다.
	DiscardCount = Discard.Num() + (DiscardTop.IsValid() ? 1 : 0);
}

bool ABaamGameState::PerformDrawCheck(int32 Seat, float SuccessChance)
{
	//	TODO : 나중에 구현
	return false;
}

void ABaamGameState::GetDeck(TArray<FBaamCardInstance>& OutDeck) const
{
	OutDeck.Empty();
	OutDeck = Deck;
}

void ABaamGameState::ShuffleDeck()
{
	for (int i = Deck.Num() - 1; i > 0; --i)
	{
		int j = RandomStream.RandRange(0, i);
		Deck.Swap(i, j);
	}
}

void ABaamGameState::ReshuffleDeck()
{	//	버림덱의 맨 위 카드(DiscardTop)를 제외하고 셔플하여 기존 덱에 추가
	if (Discard.IsEmpty())
	{
		return;
	}

	for (int i = Discard.Num() - 1; i > 0; --i)
	{
		int j = RandomStream.RandRange(0, i);
		Discard.Swap(i, j);
	}

	UE_LOG(LogBaamCard, Log, TEXT("[ReshuffleDeck] 버린 패 %d 장을 덱으로 되돌립니다."), Discard.Num());

	Deck.Append(Discard);
	Discard.Empty();
	RefreshPublicCounts();
}

