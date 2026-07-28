// BANG! 데이터 서브시스템 — DT 를 로드해 태그로 Row 를 조회한다.
// Team4Project 의 UBaseDataSubsystem 과 동일한 역할.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "Game/BaamGameDataTypes.h"
#include "BaamDataSubsystem.generated.h"

/**
 * 게임 데이터(DT) 접근 창구. GameInstance 수명과 함께 존재하므로
 * 어디서든 GetGameInstance()->GetSubsystem<UBaamDataSubsystem>() 로 접근한다.
 */
UCLASS()
class BAAM_API UBaamDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UBaamDataSubsystem();

	// 태그(= Row 이름)로 캐릭터 GAS 구성 Row 를 조회. 없으면 nullptr.
	const FBaamCharacterRow* GetCharacterRow(FGameplayTag CharacterTag) const;

	// 모든 카드 확률(가중치) Row 를 모아 반환 (확률 기반 분배용).
	void GetAllCardProbabilities(TArray<FBaamCardProbabilityRow>& OutRows) const;

protected:
	// DT 경로. 에디터에서 이 경로에 DT_BaamCharacterRow(FBaamCharacterRow) 를 만들어 둔다.
	static const FString CharacterTablePath;

	UPROPERTY()
	TObjectPtr<UDataTable> CharacterTable;

	// 카드 확률 DT 경로. 에디터에서 DT_BaamCardProbability(FBaamCardProbabilityRow) 를 만들어 둔다.
	static const FString CardProbabilityTablePath;

	UPROPERTY()
	TObjectPtr<UDataTable> CardProbabilityTable;
};
