// BANG! 게임모드 — 서버 권위. 접속 인원수(4~7)에 맞춰 역할 태그를 셔플 배정한다.
// 배정은 캐릭터의 SetCharacterTag 로 전달되고, 캐릭터가 DT(FBaamCharacterRow)를 읽어
// 스탯/능력을 스스로 부여한다. (Team4Project 의 GODGameMode::AssignRoles 대응)
//
// 판 시작 진행(트래블 도착 판정 등)은 UBaamMatchStartComponent 가 맡는다 —
// 이 클래스는 "누가 무슨 역할인가"만 안다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagContainer.h"
#include "Game/BaamGameDataTypes.h"   // EBaamDiceOutcome
#include "BaamGameMode.generated.h"

class UBaamMatchStartComponent;

UCLASS()
class BAAM_API ABaamGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABaamGameMode();

	// 지원 인원 범위 (BANG! 기본판).
	static constexpr int32 MinPlayers = 4;
	static constexpr int32 MaxPlayers = 7;

	// 서버: 접속 인원수에 맞춰 역할 태그를 셔플해 한 명씩 배정한다.
	// 인원이 4~7 밖이면 경고 로그만 남기고 아무것도 하지 않는다.
	// (호출 시점은 UBaamMatchStartComponent 가 정한다)
	UFUNCTION(BlueprintCallable, Category = "Bang")
	void AssignRoles();

	// 인원수에 맞는 역할 태그 풀을 구성 (셔플 전). 4:S/O/O/R, 5:+D, 6:+O, 7:+D.
	static TArray<FGameplayTag> BuildRolePool(int32 NumPlayers);

	// 접속한 PlayerController 를 모은다.
	void CollectPlayers(TArray<APlayerController*>& OutPlayers) const;

	// ── 주사위 판정 ──────────────────────────────────────────────
	// 주사위 면 수 (기본 24면체). 에디터/코드에서 변경 가능.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "2"))
	int32 DiceFaces = 24;

	// 주사위를 굴려 1~Faces 결과를 반환한다 (서버에서 호출). Faces<=0 이면 DiceFaces 사용.
	UFUNCTION(BlueprintCallable, Category = "Bang|Dice")
	int32 RollDice(int32 Faces = 0);

	// ── 판정 비율 (대실패/실패/성공/대성공) ──────────────────────
	// 각 단계의 "상대 비율". 합이 1 일 필요 없다(내부에서 정규화). 낮은 눈=대실패 … 높은 눈=대성공.
	// 예) 5 / 45 / 45 / 5 로 두면 극단이 5% 씩. 정수 주사위라 실제 값은 근사치.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "0.0"))
	float CriticalFailureRatio = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "0.0"))
	float FailureRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "0.0"))
	float SuccessRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "0.0"))
	float CriticalSuccessRatio = 0.1f;

	// 주어진 주사위 눈을 위 비율 구간으로 판정한다. Faces<=0 이면 DiceFaces 사용.
	UFUNCTION(BlueprintCallable, Category = "Bang|Dice")
	EBaamDiceOutcome ClassifyRoll(int32 Roll, int32 Faces = 0) const;

	// 주사위를 굴려 판정 결과를 반환한다 (굴린 눈은 OutRoll). 로그도 남긴다.
	//   RollBonus: 판정에만 더해지는 보정치(행운·민첩 등). OutRoll(원본 눈)은 그대로 두고,
	//              (OutRoll + RollBonus) 로 구간을 판정한다. 값이 커지면 상위 등급이 잘 뜬다.
	UFUNCTION(BlueprintCallable, Category = "Bang|Dice")
	EBaamDiceOutcome RollForOutcome(int32& OutRoll, int32 Faces = 0, int32 RollBonus = 0);

	// ★ 판정 결과 → 화면/로그용 텍스트 ("대실패"/"실패"/"성공"/"대성공"). 출력용.
	UFUNCTION(BlueprintPure, Category = "Bang|Dice")
	static FText GetOutcomeText(EBaamDiceOutcome Outcome);

	// ── [현재 미사용] 확률 기반 카드 분배 ────────────────────────
	//
	// ⚠️ 이 두 함수는 현재 게임 흐름에서 호출되지 않는다. 실제 카드 분배는
	//    ABaamGameState::DrawFromDeck + ABaamPlayerState::AddCardToHand 경로를 쓴다.
	//
	//    두 방식은 근본적으로 다르다:
	//      DrawWeightedCard : DT_BaamCardProbability 가중치 기반 **복원추출**.
	//                         같은 카드가 무한히 나올 수 있고 InstanceId 개념이 없다.
	//      DrawFromDeck     : 80 장 유한 덱의 **비복원추출**. 카드마다 고유 InstanceId 가 있어
	//                         손패에서 특정 카드를 지목해 제거할 수 있다.
	//
	//    섞어 쓰면 "덱에 없는 카드가 손에 들어오거나 InstanceId 가 중복되는" 버그가 난다.
	//    참고용으로 남겨두었을 뿐이므로 새 코드에서 호출하지 말 것.
	UFUNCTION(BlueprintCallable, Category = "Bang|Deck|Deprecated", meta = (DeprecatedFunction,
		DeprecationMessage = "미사용. 덱 기반 분배는 ABaamGameState::DrawFromDeck 을 사용하세요."))
	FGameplayTag DrawWeightedCard() const;

	UFUNCTION(BlueprintCallable, Category = "Bang|Deck|Deprecated", meta = (DeprecatedFunction,
		DeprecationMessage = "미사용. 손패 분배는 Baam_DealHand / DrawFromDeck + AddCardToHand 를 사용하세요."))
	void DealCards(int32 CardsPerPlayer = 5);

	// ── 콘솔 테스트 트리거 (구현: BaamCardExec.cpp) ──────────────
	UFUNCTION(Exec)
	void Baam_DumpDeck();

	UFUNCTION(Exec)
	void Baam_PrintDeck();

	// 접속한 전원에게 덱에서 Count 장씩 나눠준다 (서버 권위). 생략하면 5 장.
	UFUNCTION(Exec)
	void Baam_DealHand(int32 Count);

protected:
	// 판이 시작된 뒤의 접속은 여기서 막는다. 클라의 검색 결과는 캐시라
	// 시작 직전에 검색한 클라이언트는 bAllowJoinInProgress 검사를 통과해버린다.
	virtual void PreLogin(const FString& Options, const FString& Address,
		const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Baam")
	TObjectPtr<UBaamMatchStartComponent> MatchStart;
};
