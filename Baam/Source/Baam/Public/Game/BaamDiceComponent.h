// BANG! 확률 판정 전담 컴포넌트 — ABaamGameState 에 부착된다.
//
// 왜 GameState 인가 (GameMode 가 아니라):
//   · GameMode 는 클라에 존재하지 않는다. 판정 설정(면수/비율)을 클라가 못 읽으면
//     "성공 확률 62%" 같은 UI 를 만들 수 없다. 컴포넌트 기본값은 CDO 라 양쪽이 동일하다.
//   · 시드 난수(RandomStream)의 소유자가 GameState 다. 재현 가능한 판정을 하려면 옆에 있어야 한다.
//   · 같은 성격의 PerformDrawCheck 가 이미 GameState 에 있다 — 확률 판정을 한 곳에 모은다.
//
// ⚠️ 굴리기(RollDice/RollForOutcome)는 서버 전용이다. 클라에서 부르면 경고 후 실패값을 준다.
//    설정값 읽기와 ClassifyRoll(순수 계산)은 클라에서도 안전하다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/RandomStream.h"
#include "Game/BaamGameDataTypes.h"   // EBaamDiceOutcome
#include "BaamDiceComponent.generated.h"

UCLASS(ClassGroup = (Baam), meta = (BlueprintSpawnableComponent))
class BAAM_API UBaamDiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaamDiceComponent();

	/**
	 * 월드에서 판정 컴포넌트를 얻는다 (GameState 에 붙어 있다).
	 * 호출부가 GameState→FindComponentByClass 를 매번 쓰지 않도록 감싼 것.
	 * 클라에서도 유효하다(설정 읽기용). 없으면 nullptr.
	 */
	UFUNCTION(BlueprintPure, Category = "Bang|Dice", meta = (WorldContext = "WorldContextObject"))
	static UBaamDiceComponent* Get(const UObject* WorldContextObject);

	// ── 설정 (BP GameState 디폴트에서 튜닝) ──────────────────────
	// 주사위 면 수 (기본 24면체).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "2"))
	int32 DiceFaces = 24;

	// 각 단계의 "상대 비율". 합이 1 일 필요 없다(내부에서 정규화).
	// 낮은 눈=대실패 … 높은 눈=대성공. 예) 5/45/45/5 로 두면 극단이 5% 씩.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "0.0"))
	float CriticalFailureRatio = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "0.0"))
	float FailureRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "0.0"))
	float SuccessRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Bang|Dice", meta = (ClampMin = "0.0"))
	float CriticalSuccessRatio = 0.1f;

	// ── 시드 ─────────────────────────────────────────────────────
	/**
	 * 판정용 난수 스트림을 초기화한다 (서버 전용). GameState 가 덱 시드를 정할 때 함께 부른다.
	 *
	 * ⚠️ 셔플과 스트림을 공유하지 않는다. 한 스트림을 같이 쓰면 판정을 한 번 더 굴리는 순간
	 *    이후 셔플 결과가 통째로 밀려서, 코드를 조금만 고쳐도 같은 시드의 판이 재현되지 않고
	 *    플레이어가 판정을 유도해 다음 뽑기를 흔들 여지도 생긴다.
	 *    그래서 같은 매치 시드에서 파생하되(재현 가능) 스트림은 따로 둔다.
	 */
	void InitializeStream(int32 MatchSeed);

	// ── 판정 ─────────────────────────────────────────────────────
	// 주사위를 굴려 1~Faces 결과를 반환한다 (서버 전용). Faces<=0 이면 DiceFaces 사용.
	UFUNCTION(BlueprintCallable, Category = "Bang|Dice")
	int32 RollDice(int32 Faces = 0);

	// 주어진 눈을 비율 구간으로 판정한다 (순수 계산 — 클라에서도 안전). Faces<=0 이면 DiceFaces.
	UFUNCTION(BlueprintPure, Category = "Bang|Dice")
	EBaamDiceOutcome ClassifyRoll(int32 Roll, int32 Faces = 0) const;

	/**
	 * 주사위를 굴려 판정 결과를 반환한다 (굴린 눈은 OutRoll). 서버 전용.
	 *   RollBonus: 판정에만 더해지는 보정치(행운·민첩 등). OutRoll(원본 눈)은 그대로 두고,
	 *              (OutRoll + RollBonus) 로 구간을 판정한다. 값이 커지면 상위 등급이 잘 뜬다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Bang|Dice")
	EBaamDiceOutcome RollForOutcome(int32& OutRoll, int32 Faces = 0, int32 RollBonus = 0);

	// 판정 결과 → 화면/로그용 텍스트 ("대실패"/"실패"/"성공"/"대성공"). 출력용.
	UFUNCTION(BlueprintPure, Category = "Bang|Dice")
	static FText GetOutcomeText(EBaamDiceOutcome Outcome);

private:
	// 판정 전용 시드 스트림 (서버에서만 굴린다). 셔플 스트림과 분리 — 위 InitializeStream 주석 참고.
	FRandomStream RandomStream;

	// InitializeStream 이 아직 안 불렸으면 true. 이 상태로 굴리면 경고를 남긴다.
	bool bStreamInitialized = false;
};
