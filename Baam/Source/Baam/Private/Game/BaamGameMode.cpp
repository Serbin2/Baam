#include "Game/BaamGameMode.h"
#include "Game/BaamGameplayTags.h"
#include "Game/BaamMatchStartComponent.h"
#include "Game/BaamDataSubsystem.h"
#include "Game/BaamPlayerState.h"
#include "Network/BaamNetLog.h"
#include "Player/BaamCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Game/BaamGameState.h"

ABaamGameMode::ABaamGameMode()
{
	// 좌석/손패 등 플레이어별 데이터의 소유자(md §1.2).
	PlayerStateClass = ABaamPlayerState::StaticClass();
	GameStateClass = ABaamGameState::StaticClass();
	
	// 판 시작 진행은 컴포넌트가 맡는다.
	MatchStart = CreateDefaultSubobject<UBaamMatchStartComponent>(TEXT("MatchStart"));
}

void ABaamGameMode::PreLogin(const FString& Options, const FString& Address,
	const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	// 입장 가부 판정은 진행 컴포넌트가 갖는다 — 여기는 엔진 훅을 넘겨주기만 한다.
	if (MatchStart && !MatchStart->CanAcceptPlayer(ErrorMessage))
	{
		UE_LOG(LogBaamNet, Log, TEXT("[MatchStart] 접속 거절(%s) — %s"), *Address, *ErrorMessage);
	}
}

void ABaamGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 정원이 차면 컴포넌트가 알아서 판을 연다(md §1.4 — 로비에서 대기하다 전환).
	if (MatchStart)
	{
		MatchStart->NotifyPlayerJoined();
	}
}

TArray<FGameplayTag> ABaamGameMode::BuildRolePool(int32 NumPlayers)
{
	using namespace Bang::Role;

	TArray<FGameplayTag> Pool;

	// 공통: 보안관 1, 배신자 1.
	Pool.Add(Sheriff.GetTag());
	Pool.Add(Renegade.GetTag());

	// 무법자: 4~5인 2명 / 6~7인 3명.
	const int32 NumOutlaws = (NumPlayers <= 5) ? 2 : 3;
	for (int32 i = 0; i < NumOutlaws; ++i)
	{
		Pool.Add(Outlaw.GetTag());
	}

	// 부관: 남는 자리를 채운다 → 4인 0 / 5·6인 1 / 7인 2.
	const int32 NumDeputies = NumPlayers - Pool.Num();
	for (int32 i = 0; i < NumDeputies; ++i)
	{
		Pool.Add(Deputy.GetTag());
	}

	return Pool;
}

void ABaamGameMode::CollectPlayers(TArray<APlayerController*>& OutPlayers) const
{
	OutPlayers.Reset();

	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				OutPlayers.Add(PC);
			}
		}
	}
}

void ABaamGameMode::AssignRoles()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<APlayerController*> Players;
	CollectPlayers(Players);

	const int32 Num = Players.Num();
	if (Num < MinPlayers || Num > MaxPlayers)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Bang] AssignRoles: 접속 인원 %d 명은 지원 범위(%d~%d) 밖 — 배정 취소."),
			Num, MinPlayers, MaxPlayers);
		return;
	}

	TArray<FGameplayTag> Pool = BuildRolePool(Num);
	if (Pool.Num() != Num)
	{
		// 인원수와 풀 크기가 어긋나면(로직 오류) 잘못된 배정을 막는다.
		UE_LOG(LogTemp, Error, TEXT("[Bang] AssignRoles: 역할 풀(%d)과 인원(%d) 불일치."),
			Pool.Num(), Num);
		return;
	}

	// Fisher-Yates 셔플 (뒤에서부터 랜덤 교환).
	for (int32 i = Pool.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Pool.Swap(i, j);
	}

	// 셔플된 풀에서 한 명씩 태그를 빼서 배정.
	// SetCharacterTag 가 캐릭터의 ASC 에 DT Row(스탯 GE/패시브/어빌리티)를 부여한다.
	for (int32 i = 0; i < Num; ++i)
	{
		APlayerController* PC = Players[i];

		// 좌석은 거리 계산·대상 지정의 기준이라 역할과 함께 확정한다.
		if (ABaamPlayerState* PS = PC ? PC->GetPlayerState<ABaamPlayerState>() : nullptr)
		{
			PS->SetSeatIndex(i);
		}

		ABaamCharacter* Char = PC ? Cast<ABaamCharacter>(PC->GetPawn()) : nullptr;
		if (!Char)
		{
			// 아직 폰이 스폰되지 않은 플레이어 — 스킵 (로그로 남겨 추적).
			UE_LOG(LogTemp, Warning, TEXT("[Bang] AssignRoles: %s 의 캐릭터 없음 — 배정 스킵."),
				PC ? *PC->GetName() : TEXT("null"));
			continue;
		}

		const FGameplayTag RoleTag = Pool[i];
		Char->SetCharacterTag(RoleTag);

		// 보안관만 공개 역할이다. 나머지는 사망하거나 판이 끝날 때까지 감춘다.
		// (CharacterTag 자체는 COND_OwnerOnly 라 본인 외에는 복제되지 않는다)
		if (ABaamPlayerState* PS = PC->GetPlayerState<ABaamPlayerState>())
		{
			if (RoleTag == Bang::Role::Sheriff.GetTag())
			{
				PS->SetPublicRole(RoleTag);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[Bang] AssignRoles: %s → %s"),
			*PC->GetName(), *RoleTag.ToString());
	}
}

// ======================================================================================
//  주사위 판정
// ======================================================================================
int32 ABaamGameMode::RollDice(int32 Faces)
{
	const int32 N = (Faces > 0) ? Faces : DiceFaces;
	if (N < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] RollDice: 면 수 %d 는 2 미만 — 1 반환."), N);
		return 1;
	}

	const int32 Result = FMath::RandRange(1, N); // 1 ~ N 균등
	UE_LOG(LogTemp, Log, TEXT("[Bang] 주사위 d%d → %d"), N, Result);
	return Result;
}

EBaamDiceOutcome ABaamGameMode::ClassifyRoll(int32 Roll, int32 Faces) const
{
	const int32 N = (Faces > 0) ? Faces : DiceFaces;
	if (N < 1)
	{
		return EBaamDiceOutcome::Failure;
	}

	// 비율 정규화 (음수 방지, 합으로 나눔).
	const float CF = FMath::Max(0.f, CriticalFailureRatio);
	const float F  = FMath::Max(0.f, FailureRatio);
	const float S  = FMath::Max(0.f, SuccessRatio);
	const float CS = FMath::Max(0.f, CriticalSuccessRatio);
	const float Sum = CF + F + S + CS;
	if (Sum <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] ClassifyRoll: 비율 합이 0 — 성공으로 처리."));
		return EBaamDiceOutcome::Success;
	}

	// 굴린 눈을 (0,1] 로 정규화. 낮을수록 대실패, 높을수록 대성공.
	const float T = static_cast<float>(FMath::Clamp(Roll, 1, N)) / static_cast<float>(N);

	// 누적 구간 경계.
	const float C1 = CF / Sum;
	const float C2 = C1 + F / Sum;
	const float C3 = C2 + S / Sum;

	if (T <= C1) return EBaamDiceOutcome::CriticalFailure;
	if (T <= C2) return EBaamDiceOutcome::Failure;
	if (T <= C3) return EBaamDiceOutcome::Success;
	return EBaamDiceOutcome::CriticalSuccess;
}

EBaamDiceOutcome ABaamGameMode::RollForOutcome(int32& OutRoll, int32 Faces, int32 RollBonus)
{
	OutRoll = RollDice(Faces);

	// 보정치는 판정에만 반영한다(원본 눈 OutRoll 은 로그/연출용으로 보존).
	// ClassifyRoll 이 내부에서 [1, N] 으로 클램프하므로 초과분은 자동으로 대성공에 수렴.
	const int32 EffectiveRoll = OutRoll + RollBonus;
	const EBaamDiceOutcome Outcome = ClassifyRoll(EffectiveRoll, Faces);

	const int32 N = (Faces > 0) ? Faces : DiceFaces;
	if (RollBonus != 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Bang] 판정 — d%d 눈 %d (보정 %+d → %d) → %s"),
			N, OutRoll, RollBonus, EffectiveRoll, *GetOutcomeText(Outcome).ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Bang] 판정 — d%d 눈 %d → %s"),
			N, OutRoll, *GetOutcomeText(Outcome).ToString());
	}
	return Outcome;
}

FText ABaamGameMode::GetOutcomeText(EBaamDiceOutcome Outcome)
{
	switch (Outcome)
	{
	case EBaamDiceOutcome::CriticalFailure: return NSLOCTEXT("Bang", "DiceCritFail",    "대실패");
	case EBaamDiceOutcome::Failure:         return NSLOCTEXT("Bang", "DiceFail",        "실패");
	case EBaamDiceOutcome::Success:         return NSLOCTEXT("Bang", "DiceSuccess",     "성공");
	case EBaamDiceOutcome::CriticalSuccess: return NSLOCTEXT("Bang", "DiceCritSuccess", "대성공");
	default:                                return FText::GetEmpty();
	}
}

// ======================================================================================
//  [현재 미사용] 확률 기반 카드 분배
//
//  실제 분배 경로는 ABaamGameState::DrawFromDeck + ABaamPlayerState::AddCardToHand 다.
//  아래는 복원추출(가중치) 방식이라 InstanceId 가 없고 같은 카드가 무한히 나올 수 있어
//  덱 기반 시스템과 섞이면 안 된다. 참고용으로만 남긴다 — 호출처 없음.
// ======================================================================================
FGameplayTag ABaamGameMode::DrawWeightedCard() const
{
	const UGameInstance* GI = GetGameInstance();
	const UBaamDataSubsystem* DS = GI ? GI->GetSubsystem<UBaamDataSubsystem>() : nullptr;
	if (!DS)
	{
		return FGameplayTag();
	}

	TArray<FBaamCardProbabilityRow> Rows;
	DS->GetAllCardProbabilities(Rows);
	if (Rows.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] DrawWeightedCard: 카드 확률 데이터 없음(DT 비었거나 미지정)."));
		return FGameplayTag();
	}

	// 가중치 합 (음수 방지).
	float Total = 0.f;
	for (const FBaamCardProbabilityRow& Row : Rows)
	{
		Total += FMath::Max(0.f, Row.Weight);
	}
	if (Total <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] DrawWeightedCard: 가중치 합이 0 — 뽑을 카드 없음."));
		return FGameplayTag();
	}

	// [0, Total) 난수를 누적 가중치 구간에 대응시켜 선택.
	const float Pick = FMath::FRandRange(0.f, Total);
	float Cumulative = 0.f;
	for (const FBaamCardProbabilityRow& Row : Rows)
	{
		Cumulative += FMath::Max(0.f, Row.Weight);
		if (Pick < Cumulative)
		{
			return Row.CardId;
		}
	}

	return Rows.Last().CardId; // 부동소수 오차 보정 — 마지막 카드로.
}

void ABaamGameMode::DealCards(int32 CardsPerPlayer)
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<APlayerController*> Players;
	CollectPlayers(Players);

	for (APlayerController* PC : Players)
	{
		TArray<FString> Dealt;
		Dealt.Reserve(CardsPerPlayer);

		for (int32 i = 0; i < CardsPerPlayer; ++i)
		{
			const FGameplayTag Card = DrawWeightedCard();
			Dealt.Add(Card.IsValid() ? Card.ToString() : TEXT("(없음)"));

			// TODO(손패 시스템): 여기서 뽑은 Card 를 이 플레이어의 PlayerState.Hand 에 추가.
		}

		UE_LOG(LogTemp, Log, TEXT("[Bang] 카드 분배 — %s: %s"),
			PC ? *PC->GetName() : TEXT("null"), *FString::Join(Dealt, TEXT(", ")));
	}
}
