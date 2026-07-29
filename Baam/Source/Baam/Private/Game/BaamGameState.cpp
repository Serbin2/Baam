// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BaamGameState.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/GameInstance.h"
#include "Game/BaamCardLog.h"
#include "Game/BaamDataSubsystem.h"
#include "Game/BaamGameplayTags.h"
#include "Game/BaamPlayerState.h"
#include "Player/BaamCharacter.h"
#include "Player/Component/BaamAttributeSet.h"
#include "Net/UnrealNetwork.h"

void ABaamGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABaamGameState, DeckCount);
	DOREPLIFETIME(ABaamGameState, DiscardCount);
	DOREPLIFETIME(ABaamGameState, DiscardTop);
	DOREPLIFETIME(ABaamGameState, PhaseTag);
	DOREPLIFETIME(ABaamGameState, CurrentSeat);
	DOREPLIFETIME(ABaamGameState, WinningRoleTag);
}

FGameplayTag ABaamGameState::GameOverPhaseTag()
{
	return Bang::Phase::GameOver.GetTag();
}

void ABaamGameState::BeginPlay()
{
	Super::BeginPlay();

	//	덱은 서버만 만든다. 클라의 Deck 배열은 영원히 비어 있는 것이 정상
	if (HasAuthority())
	{
		InitializeDeck();
	}
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

// ══════════════════════════════════════════════════════════════════════════════════════
//  좌석 조회
//
//  "살아 있다" 의 정의를 여기 한곳에만 둔다. 턴 진행과 거리 계산이 서로 다른 기준을 쓰면
//  UI 는 살아 있다고 보는데 턴은 건너뛰는 식의 추적하기 어려운 버그가 난다.
// ══════════════════════════════════════════════════════════════════════════════════════

ABaamPlayerState* ABaamGameState::GetPlayerStateBySeat(int32 Seat) const
{
	if (Seat == INDEX_NONE)
	{
		return nullptr;
	}

	for (APlayerState* Base : PlayerArray)
	{
		ABaamPlayerState* PS = Cast<ABaamPlayerState>(Base);
		if (PS && PS->GetSeatIndex() == Seat)
		{
			return PS;
		}
	}
	return nullptr;
}

int32 ABaamGameState::GetSeatHealth(int32 Seat) const
{
	const ABaamPlayerState* PS = GetPlayerStateBySeat(Seat);
	const UAbilitySystemComponent* ASC =
		PS ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS->GetPawn()) : nullptr;

	return ASC ? FMath::RoundToInt(ASC->GetNumericAttribute(UBaamAttributeSet::GetHealthAttribute())) : 0;
}

bool ABaamGameState::IsSeatAlive(int32 Seat) const
{
	const ABaamPlayerState* PS = GetPlayerStateBySeat(Seat);
	if (!PS)
	{
		return false;
	}

	//	사망은 PlayerState 의 복제 변수가 정본이다 — 서버·클라가 같은 답을 내야 한다.
	//	(ASC 의 State.Dead 루즈 태그는 복제되지 않아 클라에서 항상 false 다)
	if (PS->IsDead())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS->GetPawn());
	if (!ASC)
	{
		//	폰이 아직 스폰되지 않았다 — 살아 있는 것으로 본다(판 시작 직후 등).
		return true;
	}

	return ASC->GetNumericAttribute(UBaamAttributeSet::GetHealthAttribute()) > 0.f;
}

TArray<int32> ABaamGameState::GetAliveSeatsInTableOrder() const
{
	TArray<int32> Seats;

	for (APlayerState* Base : PlayerArray)
	{
		const ABaamPlayerState* PS = Cast<ABaamPlayerState>(Base);
		if (PS && PS->GetSeatIndex() != INDEX_NONE && IsSeatAlive(PS->GetSeatIndex()))
		{
			Seats.Add(PS->GetSeatIndex());
		}
	}

	Seats.Sort();
	return Seats;
}

FGameplayTag ABaamGameState::GetSeatRole(int32 Seat) const
{
	const ABaamPlayerState* PS = GetPlayerStateBySeat(Seat);
	const ABaamCharacter* Char = PS ? Cast<ABaamCharacter>(PS->GetPawn()) : nullptr;
	return Char ? Char->GetCharacterTag() : FGameplayTag();
}

int32 ABaamGameState::GetHandLimitForSeat(int32 Seat) const
{
	//	뱅 규칙: 손패 한도는 "현재" 생명력이다(최대치가 아니다).
	return GetSeatHealth(Seat);
}

// ══════════════════════════════════════════════════════════════════════════════════════
//  턴 진행
// ══════════════════════════════════════════════════════════════════════════════════════

void ABaamGameState::SetPhase(const FGameplayTag& NewPhase)
{
	PhaseTag = NewPhase;
	UE_LOG(LogBaamCard, Log, TEXT("[Turn] 좌석 %d — %s"), CurrentSeat, *NewPhase.ToString());
}

void ABaamGameState::StartMatch()
{
	if (!HasAuthority() || bMatchRunning)
	{
		return;
	}

	const TArray<int32> Alive = GetAliveSeatsInTableOrder();
	if (Alive.IsEmpty())
	{
		UE_LOG(LogBaamCard, Error, TEXT("[Turn] StartMatch: 좌석이 배정된 플레이어가 없습니다."));
		return;
	}

	bMatchRunning = true;

	//	초기 손패. InitialHandSize 가 0 이하면 좌석의 Health 만큼 준다(뱅 원작).
	for (const int32 Seat : Alive)
	{
		ABaamPlayerState* PS = GetPlayerStateBySeat(Seat);
		if (!PS)
		{
			continue;
		}

		const int32 Count = (InitialHandSize > 0) ? InitialHandSize : FMath::Max(1, GetSeatHealth(Seat));
		for (int32 i = 0; i < Count; ++i)
		{
			FBaamCardInstance Card;
			if (!DrawFromDeck(Card))
			{
				break;
			}
			PS->AddCardToHand(Card);
		}

		UE_LOG(LogBaamCard, Log, TEXT("[Turn] 초기 손패 — 좌석 %d : %d 장"), Seat, PS->GetHandCount());
	}

	BeginTurn(Alive[0]);
}

void ABaamGameState::BeginTurn(int32 Seat)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentSeat = Seat;
	SetPhase(Bang::Phase::TurnStart.GetTag());

	//	TODO: 감옥/다이너마이트 뽑기 판정이 여기 들어간다(PerformDrawCheck).
	//	      지금은 통과 페이즈다.

	EnterDrawPhase();
}

void ABaamGameState::EnterDrawPhase()
{
	SetPhase(Bang::Phase::Draw.GetTag());

	ABaamPlayerState* PS = GetPlayerStateBySeat(CurrentSeat);
	if (!PS)
	{
		EnterPlayPhase();
		return;
	}

	//	뽑을 장수는 어트리뷰트 DrawCount 에서 읽는다(Kit Carlson 등 캐릭터가 바꿀 수 있다).
	int32 DrawNum = DefaultDrawCount;
	if (const UAbilitySystemComponent* ASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS->GetPawn()))
	{
		const int32 Attr = FMath::RoundToInt(ASC->GetNumericAttribute(UBaamAttributeSet::GetDrawCountAttribute()));
		if (Attr > 0)
		{
			DrawNum = Attr;
		}
	}

	int32 Drawn = 0;
	for (int32 i = 0; i < DrawNum; ++i)
	{
		FBaamCardInstance Card;
		if (!DrawFromDeck(Card))
		{
			UE_LOG(LogBaamCard, Warning, TEXT("[Turn] 덱 고갈 — 좌석 %d 은 %d 장만 뽑음"), CurrentSeat, Drawn);
			break;
		}
		PS->AddCardToHand(Card);
		++Drawn;
	}

	UE_LOG(LogBaamCard, Log, TEXT("[Turn] 좌석 %d 뽑기 %d 장 (손패 %d, 덱 %d)"),
		CurrentSeat, Drawn, PS->GetHandCount(), DeckCount);

	EnterPlayPhase();
}

void ABaamGameState::EnterPlayPhase()
{
	SetPhase(Bang::Phase::Play.GetTag());
}

bool ABaamGameState::CanSeatPlayCards(int32 Seat) const
{
	return bMatchRunning
		&& Seat != INDEX_NONE
		&& Seat == CurrentSeat
		&& PhaseTag == Bang::Phase::Play.GetTag();
}

bool ABaamGameState::CanSeatDiscard(int32 Seat) const
{
	return bMatchRunning
		&& Seat != INDEX_NONE
		&& Seat == CurrentSeat
		&& PhaseTag == Bang::Phase::Discard.GetTag();
}

void ABaamGameState::RequestEndTurn(int32 RequestingSeat)
{
	if (!HasAuthority() || !bMatchRunning)
	{
		return;
	}

	//	자기 턴이 아니면 무시. 클라가 마음대로 턴을 넘기지 못하게 한다.
	if (RequestingSeat != CurrentSeat || PhaseTag != Bang::Phase::Play.GetTag())
	{
		UE_LOG(LogBaamCard, Warning,
			TEXT("[Turn] EndTurn 거부 — 요청 좌석 %d, 현재 좌석 %d, 페이즈 %s"),
			RequestingSeat, CurrentSeat, *PhaseTag.ToString());
		return;
	}

	const ABaamPlayerState* PS = GetPlayerStateBySeat(CurrentSeat);
	const int32 HandNum  = PS ? PS->GetHandCount() : 0;
	const int32 Limit    = GetHandLimitForSeat(CurrentSeat);

	if (HandNum > Limit)
	{
		//	한도를 넘었다 — 버릴 때까지 턴을 넘기지 않는다.
		SetPhase(Bang::Phase::Discard.GetTag());
		UE_LOG(LogBaamCard, Log, TEXT("[Turn] 좌석 %d 손패 초과 (%d > 한도 %d) — 버리기 대기"),
			CurrentSeat, HandNum, Limit);
		return;
	}

	AdvanceToNextSeat();
}

void ABaamGameState::NotifyCardDiscarded(int32 Seat)
{
	if (!HasAuthority() || !CanSeatDiscard(Seat))
	{
		return;
	}

	const ABaamPlayerState* PS = GetPlayerStateBySeat(Seat);
	const int32 HandNum = PS ? PS->GetHandCount() : 0;
	const int32 Limit   = GetHandLimitForSeat(Seat);

	if (HandNum <= Limit)
	{
		AdvanceToNextSeat();
	}
}

// ══════════════════════════════════════════════════════════════════════════════════════
//  사망 / 승패
// ══════════════════════════════════════════════════════════════════════════════════════

void ABaamGameState::HandleSeatDeath(int32 DeadSeat, int32 KillerSeat)
{
	if (!HasAuthority())
	{
		return;
	}

	const FGameplayTag DeadRole   = GetSeatRole(DeadSeat);
	const FGameplayTag KillerRole = GetSeatRole(KillerSeat);

	// ── 1. 사망자의 역할을 공개하고 손패·장비를 전부 버린 패로 ──
	if (ABaamPlayerState* DeadPS = GetPlayerStateBySeat(DeadSeat))
	{
		//	뱅 규칙: 죽으면 역할이 공개된다. 이 시점부터 전원이 볼 수 있다.
		DeadPS->SetPublicRole(DeadRole);

		TArray<FBaamCardInstance> Dropped;
		DeadPS->TakeAllCards(Dropped);
		for (const FBaamCardInstance& Card : Dropped)
		{
			PushToDiscard(Card);
		}

		UE_LOG(LogBaamCard, Log, TEXT("[Death] 좌석 %d(%s) 카드 %d 장 버림"),
			DeadSeat, *DeadRole.ToString(), Dropped.Num());
	}

	// ── 2. 보상 / 벌칙 (뱅 원작) ──
	if (KillerSeat != INDEX_NONE && KillerSeat != DeadSeat && IsSeatAlive(KillerSeat))
	{
		if (DeadRole == Bang::Role::Outlaw.GetTag())
		{
			// 무법자를 잡으면 현상금 3장.
			ABaamPlayerState* KillerPS = GetPlayerStateBySeat(KillerSeat);
			int32 Drawn = 0;
			for (int32 i = 0; KillerPS && i < OutlawKillReward; ++i)
			{
				FBaamCardInstance Card;
				if (!DrawFromDeck(Card))
				{
					break;
				}
				KillerPS->AddCardToHand(Card);
				++Drawn;
			}
			UE_LOG(LogBaamCard, Log, TEXT("[Death] 무법자 사살 보상 — 좌석 %d 가 %d 장 획득"), KillerSeat, Drawn);
		}
		else if (DeadRole == Bang::Role::Deputy.GetTag() && KillerRole == Bang::Role::Sheriff.GetTag())
		{
			// 보안관이 부관을 쐈다 — 손패와 장비를 전부 잃는다.
			if (ABaamPlayerState* SheriffPS = GetPlayerStateBySeat(KillerSeat))
			{
				TArray<FBaamCardInstance> Lost;
				SheriffPS->TakeAllCards(Lost);
				for (const FBaamCardInstance& Card : Lost)
				{
					PushToDiscard(Card);
				}
				UE_LOG(LogBaamCard, Log, TEXT("[Death] 보안관이 부관 사살 — 좌석 %d 카드 %d 장 몰수"),
					KillerSeat, Lost.Num());
			}
		}
	}

	// ── 3. 승패 판정 ──
	if (CheckWinCondition())
	{
		return;   // 판이 끝났으면 턴을 넘기지 않는다.
	}

	// ── 4. 죽은 사람이 현재 턴이었다면 턴을 넘긴다 ──
	if (DeadSeat == CurrentSeat && bMatchRunning)
	{
		AdvanceToNextSeat();
	}
}

bool ABaamGameState::CheckWinCondition()
{
	if (!bMatchRunning)
	{
		return false;
	}

	const TArray<int32> Alive = GetAliveSeatsInTableOrder();

	int32 NumSheriff = 0, NumDeputy = 0, NumOutlaw = 0, NumRenegade = 0;
	for (const int32 Seat : Alive)
	{
		//	'Role' 은 AActor 의 멤버명이라 섀도잉을 피한다 (C4458 은 에러로 승격됨).
		const FGameplayTag SeatRole = GetSeatRole(Seat);
		if      (SeatRole == Bang::Role::Sheriff.GetTag())  { ++NumSheriff; }
		else if (SeatRole == Bang::Role::Deputy.GetTag())   { ++NumDeputy; }
		else if (SeatRole == Bang::Role::Outlaw.GetTag())   { ++NumOutlaw; }
		else if (SeatRole == Bang::Role::Renegade.GetTag()) { ++NumRenegade; }
	}

	FGameplayTag Winner;

	if (NumSheriff == 0)
	{
		// 보안관이 죽었다. 배신자가 마지막 1인으로 남았으면 배신자 승, 아니면 무법자 승.
		Winner = (Alive.Num() == 1 && NumRenegade == 1)
			? Bang::Role::Renegade.GetTag()
			: Bang::Role::Outlaw.GetTag();
	}
	else if (NumOutlaw == 0 && NumRenegade == 0)
	{
		// 보안관 생존 + 적대 진영 전멸 → 보안관 진영 승.
		Winner = Bang::Role::Sheriff.GetTag();
	}

	if (!Winner.IsValid())
	{
		return false;
	}

	WinningRoleTag = Winner;
	CurrentSeat    = INDEX_NONE;
	bMatchRunning  = false;
	SetPhase(Bang::Phase::GameOver.GetTag());

	//	판이 끝나면 전원의 역할을 공개한다(누가 배신자였는지 보는 것이 마무리의 재미다).
	for (APlayerState* Base : PlayerArray)
	{
		if (ABaamPlayerState* PS = Cast<ABaamPlayerState>(Base))
		{
			PS->SetPublicRole(GetSeatRole(PS->GetSeatIndex()));
		}
	}

	UE_LOG(LogBaamCard, Warning, TEXT("[Match] 판 종료 — 승리 진영 : %s (생존 %d명)"),
		*Winner.ToString(), Alive.Num());
	return true;
}

void ABaamGameState::AdvanceToNextSeat()
{
	const TArray<int32> Alive = GetAliveSeatsInTableOrder();
	if (Alive.IsEmpty())
	{
		//	전멸 — 승패 판정이 붙기 전까지는 여기서 멈춘다.
		UE_LOG(LogBaamCard, Warning, TEXT("[Turn] 살아있는 좌석이 없습니다 — 진행 중단."));
		SetPhase(Bang::Phase::GameOver.GetTag());
		bMatchRunning = false;
		return;
	}

	//	현재 좌석이 죽어서 목록에 없을 수 있다. 그때는 "다음으로 큰 좌석" 을 찾는다.
	int32 NextIndex = 0;
	const int32 Found = Alive.IndexOfByKey(CurrentSeat);
	if (Found != INDEX_NONE)
	{
		NextIndex = (Found + 1) % Alive.Num();
	}
	else
	{
		for (int32 i = 0; i < Alive.Num(); ++i)
		{
			if (Alive[i] > CurrentSeat)
			{
				NextIndex = i;
				break;
			}
		}
	}

	BeginTurn(Alive[NextIndex]);
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

