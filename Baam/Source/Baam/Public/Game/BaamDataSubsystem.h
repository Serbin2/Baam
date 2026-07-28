// BANG! 데이터 서브시스템 — DT 를 로드해 태그로 Row 를 조회한다.
// Team4Project 의 UBaseDataSubsystem 과 동일한 역할.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "Game/BaamGameDataTypes.h"
#include "BaamDataSubsystem.generated.h"

struct FBaamCardInstance;
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
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	const FBaamCardRow* GetCardRow(FName CardID) const;
	const FBaamCardRow* GetCardRow(FGameplayTag CardTag) const;

protected:
	// DT 경로. 에디터에서 이 경로에 DT_BaamCharacterRow(FBaamCharacterRow) 를 만들어 둔다.
	static const FString CharacterTablePath;

	UPROPERTY()
	TObjectPtr<UDataTable> CharacterTable;
	
	/** DT_BaamCard 의 QuantityOfCard 을 읽어 종류별 매수만큼 카드를 전개. InstanceId 는 1부터. */
	void BuildDeck(TArray<FBaamCardInstance>& OutDeck) const;
	
	UPROPERTY()
	TArray<FBaamCardInstance> Deck;
	
	UPROPERTY()
	TObjectPtr<UDataTable> CardTable;
};
