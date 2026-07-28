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
