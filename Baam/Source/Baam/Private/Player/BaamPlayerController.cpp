#include "Player/BaamPlayerController.h"
#include "Player/Ability/GA_Bang.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemGlobals.h"

ABaamPlayerController::ABaamPlayerController()
{
	// 1단계 기본값 — 이후 카드 데이터에서 GA 를 조회하게 바뀐다.
	BangAbilityClass = UGA_Bang::StaticClass();
}

void ABaamPlayerController::RequestPlayCard(int32 InstanceId, int32 TargetSeat)
{
	// 로컬에서는 검증하지 않는다 (클라 표시는 힌트일 뿐, 권위는 서버). 그대로 중계.
	ServerRequestPlayCard(InstanceId, TargetSeat);
}

bool ABaamPlayerController::ServerRequestPlayCard_Validate(int32 InstanceId, int32 TargetSeat)
{
	// 형식 검증만 (악의적 페이로드 차단). 규칙 검증은 _Implementation 에서.
	return InstanceId != INDEX_NONE;
}

void ABaamPlayerController::ServerRequestPlayCard_Implementation(int32 InstanceId, int32 TargetSeat)
{
	HandlePlayCard(InstanceId, TargetSeat);
}

void ABaamPlayerController::HandlePlayCard(int32 InstanceId, int32 TargetSeat)
{
	// TODO(2단계): GameState 규칙 검증 — 내 턴? Phase.Play? 사거리? BangLimit?
	UE_LOG(LogTemp, Log, TEXT("[Bang] ServerRequestPlayCard 수신 — PC=%s Instance=%d TargetSeat=%d"),
		*GetName(), InstanceId, TargetSeat);

	// 시전자의 ASC 를 얻는다 (현재 ASC 는 캐릭터 소유).
	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn());
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] HandlePlayCard: 시전자 ASC 없음 (Pawn=%s)."),
			GetPawn() ? *GetPawn()->GetName() : TEXT("null"));
		return;
	}

	if (!BangAbilityClass)
	{
		return;
	}

	// 카드 주도 발동 (md §1.1): 카드가 지정한 메커니즘 GA 를 즉석 부여+발동.
	// TODO(2단계): InstanceId → 카드 데이터에서 GA/파라미터/대상요구를 조회해 분기.
	FGameplayAbilitySpec Spec(BangAbilityClass, /*Level=*/1, /*InputID=*/INDEX_NONE, /*SourceObject=*/this);
	ASC->GiveAbilityAndActivateOnce(Spec);
}
