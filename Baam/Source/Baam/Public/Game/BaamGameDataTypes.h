// BANG! 데이터 테이블 Row 타입 모음 (Team4Project 의 BaseGameDataTypes 대응)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "BaamGameDataTypes.generated.h"

class UGameplayAbility;
class UGameplayEffect;

/**
 * 주사위 판정 결과 4단계. 낮은 눈 = 대실패, 높은 눈 = 대성공.
 * 각 단계의 폭은 GameMode 의 비율(Ratio) 프로퍼티로 조절한다.
 */
UENUM(BlueprintType)
enum class EBaamDiceOutcome : uint8
{
	CriticalFailure UMETA(DisplayName = "대실패"),
	Failure         UMETA(DisplayName = "실패"),
	Success         UMETA(DisplayName = "성공"),
	CriticalSuccess UMETA(DisplayName = "대성공")
};

/**
 * 4단계 판정의 상대 비율 (GDD §4.2 / §4.3).
 *
 * 합이 100 일 필요는 없다 — 시스템이 정규화한다. 기획은 상대값으로 입력한다.
 * 보정(캐릭터·장비·상태)은 이 비율에 더해진 뒤 정규화된다.
 */
USTRUCT(BlueprintType)
struct FBaamOutcomeWeights
{
	GENERATED_BODY()

	//	GDD §4.4: 대실패 비율은 낮게 유지한다. 통제 불능이라는 인상을 주면 공격 자체가 위축된다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 CriticalFailure = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 Failure = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 Success = 65;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 CriticalSuccess = 10;

	int32 Sum() const { return CriticalFailure + Failure + Success + CriticalSuccess; }

	/** 보정을 더한다. 음수로 내려간 항목은 0 으로 클램프한다 (GDD §4.2). */
	void ApplyBonus(int32 SuccessBonus, int32 CriticalBonus)
	{
		Success         = FMath::Max(0, Success + SuccessBonus);
		CriticalSuccess = FMath::Max(0, CriticalSuccess + CriticalBonus);
	}
};

/**
 * 판정 등급별 효과 수치 (GDD §4.3). 카드에 따라 피해량·회복량·뽑는 장수 등으로 해석된다.
 *
 * GDD §4.1: `실패`는 효과가 발생하지 않는 것이 기본안이므로 0 이다.
 * GDD §4.4: `대실패`가 모든 카드에 불이익을 강제하지 않는다 — 필요 없으면 0 으로 둔다.
 */
USTRUCT(BlueprintType)
struct FBaamOutcomeMagnitudes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CriticalFailure = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Failure = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Success = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CriticalSuccess = 2;

	int32 ForOutcome(EBaamDiceOutcome Outcome) const
	{
		switch (Outcome)
		{
		case EBaamDiceOutcome::CriticalSuccess: return CriticalSuccess;
		case EBaamDiceOutcome::Success:         return Success;
		case EBaamDiceOutcome::Failure:         return Failure;
		case EBaamDiceOutcome::CriticalFailure: return CriticalFailure;
		default:                                return 0;
		}
	}
};

/**
 * 캐릭터(또는 역할) 1종의 GAS 구성을 서술하는 DT Row.
 *   - Row 이름 = CharacterTag 의 태그 문자열 (예: "Character.Ability.PaulRegret").
 *     DataSubsystem 이 태그로 이 Row 를 찾아 캐릭터에 부여한다.
 *
 * BANG! 매핑
 *   DefaultAttributeGE  : Health/WeaponRange/BangLimit 등 기본 스탯을 세팅하는 GE(Instant).
 *   PassiveEffects      : 캐릭터 특능 패시브 GE(Infinite). 예) Paul Regret → DistanceIncrease +1.
 *   GrantedAbilities    : 이 캐릭터가 쓸 수 있는 능동 어빌리티(GA). BANG! 에서는 대부분
 *                         카드가 GA 를 구동하지만, 캐릭터 고유 능동기(Sid Ketchum 등)는 여기서 부여.
 */
USTRUCT(BlueprintType)
struct FBaamCharacterRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 이 Row 를 식별하는 태그 (Role.* 또는 Character.Ability.*).
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag CharacterTag;

	// 기본 스탯 세팅 GE (Instant). Health/MaxHealth/WeaponRange/BangLimit ... 를 초기화.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DefaultAttributeGE;

	// 캐릭터 특수능력 패시브 GE (Infinite). 역할 변경 시 함께 회수된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UGameplayEffect>> PassiveEffects;

	// 이 캐릭터에게 부여할 능동 어빌리티 (GA).
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;
};

/**
 * 카드 1종의 등장 확률(가중치). 확률 기반 카드 분배용.
 *   - Row 이름 = CardId 태그 문자열 (예: "Card.Id.Bang") 로 두면 관리가 편하다.
 *   - Weight 는 "상대 가중치" — 전부 합해서 1 일 필요 없다. 클수록 자주 뽑히고, 0 이면 안 나온다.
 *     예) Bang=25, Missed=12, Beer=6 처럼 넣으면 실제 덱 비율과 비슷해진다.
 */
USTRUCT(BlueprintType)
struct FBaamCardProbabilityRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 어떤 카드인가 (Card.Id.* 태그).
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag CardId;

	// 뽑힐 상대 가중치(확률). 0 이면 등장하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};
USTRUCT(BlueprintType)
struct FBaamCardRow : public FTableRowBase
{
	GENERATED_BODY()
public:
	//	카드 종류 ( Card.Id.* )
	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
	FGameplayTag CardIdTag;
	
	//	카드 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
	FText DisplayName;
	//	카드 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
	FText Description;
	
	//	카드 타입 ( Card.Type.* )
	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
	FGameplayTag TypeTag;
	
	//	카드 특성 ( Card.Trait.* )
	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
	FGameplayTagContainer TraitTags;

	// 갈색 카드: 사용 시 이 이벤트 태그로 GA 를 트리거한다 (Ability.Bang 등).
	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
	FGameplayTag AbilityEventTag;

	// 파란 카드: 장착 시 적용할 Infinite GE.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
	TSubclassOf<UGameplayEffect> EquipEffect;

	// GA/GE 가 읽는 수치. 역마차 2 / 웰스파고 3 / 무기 사거리 / 피해량 등.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) 
	int32 Magnitude = 0;
	
	//	덱에 들어갈 카드 매수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 QuantityOfCard = 0;
	
	//	[비활성] 확률을 사용하는 카드의 성공 판정 기준값.
	//	  OutcomeWeights 로 대체됐다 (GDD §4.2 — 4단계 비율 방식). 참조하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 CheckChance = 0;

	// ── 4단계 판정 (GDD §4.2 / §4.3) ──────────────────────────────

	//	판정 비율. 합이 100 일 필요 없다(정규화됨).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outcome")
	FBaamOutcomeWeights OutcomeWeights;

	//	등급별 효과 수치. 카드에 따라 피해량·회복량·장수 등으로 해석된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outcome")
	FBaamOutcomeMagnitudes OutcomeMagnitudes;

	//	이 카드가 받을 수 있는 보정의 종류 (GDD §4.3 "보정 허용 태그").
	//	비어 있으면 모든 보정을 받는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outcome")
	FGameplayTagContainer ModifierTags;
};
