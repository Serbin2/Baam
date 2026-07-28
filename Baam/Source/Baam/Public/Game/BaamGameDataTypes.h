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
	
	//	확률을 사용하는 카드의 경우, 성공 판정을 받기 위해 필요한 성공값 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 CheckChance = 0;
};