#include "Game/BaamDiceComponent.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

UBaamDiceComponent::UBaamDiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UBaamDiceComponent* UBaamDiceComponent::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	return GS ? GS->FindComponentByClass<UBaamDiceComponent>() : nullptr;
}

void UBaamDiceComponent::InitializeStream(int32 MatchSeed)
{
	//	같은 매치 시드에서 파생시키되 셔플과는 다른 수열을 쓴다(재현 가능 + 상호 간섭 없음).
	//	곱하는 상수는 홀수 소수 — 시드가 1씩 달라져도 수열이 충분히 벌어진다.
	const int32 DiceSeed = MatchSeed * 2654435761u + 1013904223;

	RandomStream.Initialize(DiceSeed);
	bStreamInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("[Bang] 주사위 스트림 초기화 — 매치시드 %d → 판정시드 %d"),
		MatchSeed, DiceSeed);
}

int32 UBaamDiceComponent::RollDice(int32 Faces)
{
	//	굴리기는 서버 권위다. 클라에서 굴리면 서버와 결과가 갈라진다.
	const AActor* Owner = GetOwner();
	if (Owner && !Owner->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] RollDice 는 서버 전용이다 — 클라 호출 무시(1 반환)."));
		return 1;
	}

	const int32 N = (Faces > 0) ? Faces : DiceFaces;
	if (N < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] RollDice: 면 수 %d 는 2 미만 — 1 반환."), N);
		return 1;
	}

	if (!bStreamInitialized)
	{
		//	InitializeStream 전에 굴리면 매 판 같은 수열이 나온다 — 초기화 누락을 알린다.
		UE_LOG(LogTemp, Warning,
			TEXT("[Bang] 주사위 스트림이 초기화되지 않았다 — GameState 의 덱 초기화가 먼저 돌아야 한다."));
	}

	const int32 Result = RandomStream.RandRange(1, N); // 1 ~ N 균등
	UE_LOG(LogTemp, Log, TEXT("[Bang] 주사위 d%d → %d"), N, Result);
	return Result;
}

EBaamDiceOutcome UBaamDiceComponent::ClassifyRoll(int32 Roll, int32 Faces) const
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

EBaamDiceOutcome UBaamDiceComponent::RollForOutcome(int32& OutRoll, int32 Faces, int32 RollBonus)
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

FText UBaamDiceComponent::GetOutcomeText(EBaamDiceOutcome Outcome)
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
