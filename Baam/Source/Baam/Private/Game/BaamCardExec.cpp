// BANG! 카드/덱 콘솔 테스트 트리거. Network/Session/BaamSessionExec.cpp 와 같은 관례:
//   선언은 헤더에 항상 남기고(UFUNCTION(Exec)), 본문만 #if !UE_BUILD_SHIPPING 으로 감싼다.
//
// 호스트 클래스 선택 메모 (Docs/Card-Implementation-Workflow.md §exec 표):
//   Baam_DumpDeck 은 DT 를 읽기만 하고 권한이 무관하므로 GameInstance 가 맞다 — 어느 PIE 창에서나 동작한다.
//   반대로 서버 권위 조작(카드 배분, 피해 적용)은 ABaamGameMode 에 올려야 한다.
//   World->GetAuthGameMode() 가 클라에서 null 이라, 클라 창에서는 명령이 해석조차 되지 않아
//   "왜 안 되는지" 가 즉시 드러난다.

#include "Game/BaamGameInstance.h"

#include "Game/BaamCardLog.h"
#include "Game/BaamCardType.h"
#include "Game/BaamDataSubsystem.h"
#include "Game/BaamGameDataTypes.h"

// BaamGameDataTypes.h 는 UGameplayEffect 를 전방 선언만 한다.
// TSubclassOf<UGameplayEffect> 를 bool 로 평가하면 UGameplayEffect::StaticClass() 가 필요하므로
// 완전한 타입이 있어야 한다.
#include "GameplayEffect.h"

void UBaamGameInstance::Baam_DumpDeck()
{
#if !UE_BUILD_SHIPPING
	// 기본판 덱은 정확히 80장 — QuantityOfCard 합계의 자체 검증 기준.
	static constexpr int32 ExpectedDeckSize = 80;

	const UBaamDataSubsystem* Data = GetSubsystem<UBaamDataSubsystem>();
	if (!Data)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] BaamDataSubsystem 없음"));
		return;
	}

	// Deck 은 Initialize() 에서 한 번 만들어 캐시된다. 여기서 다시 만들지 않으므로
	// 두 번 연속 실행하면 항상 같은 결과가 나와야 한다.
	const TArray<FBaamCardInstance> Deck = Data->GetDeck();
	if (Deck.IsEmpty())
	{
		UE_LOG(LogBaamCard, Warning,
			TEXT("[Exec] 덱이 비어 있습니다. DT_BaamCard 의 QuantityOfCard 가 전부 0 이거나 "
			     "프로젝트 설정(Baam Data ▸ CardTable)이 비어 있습니다."));
		return;
	}

	// 종류별 집계 + InstanceId 중복 검사를 한 번에.
	TMap<FName, int32> Counts;
	TSet<int32> SeenIds;
	int32 NumDuplicateIds = 0;

	Counts.Reserve(Deck.Num());
	SeenIds.Reserve(Deck.Num());

	for (const FBaamCardInstance& Card : Deck)
	{
		++Counts.FindOrAdd(Card.CardId);

		bool bAlreadySeen = false;
		SeenIds.Add(Card.InstanceId, &bAlreadySeen);
		if (bAlreadySeen)
		{
			++NumDuplicateIds;
		}
	}

	Counts.KeySort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });

	UE_LOG(LogBaamCard, Log, TEXT("===== Baam_DumpDeck ====="));

	int32 NumUnimplemented = 0;
	for (const TPair<FName, int32>& Pair : Counts)
	{
		// GA(AbilityEventTag) 도 GE(EquipEffect) 도 없는 카드가 덱에 들어가 있으면
		// 손패에는 나오지만 사용해도 아무 일이 일어나지 않는다. 구현 진행도 추적용.
		const FBaamCardRow* Row = Data->GetCardRow(Pair.Key);
		const bool bUnimplemented = (Row != nullptr) && !Row->AbilityEventTag.IsValid() && !Row->EquipEffect;
		if (bUnimplemented)
		{
			++NumUnimplemented;
		}

		UE_LOG(LogBaamCard, Log, TEXT("  %-32s %3d 장%s"),
			*Pair.Key.ToString(),
			Pair.Value,
			(Row == nullptr)  ? TEXT("   [!! DT_BaamCard 에 Row 없음]") :
			bUnimplemented    ? TEXT("   [미구현: GA/GE 미지정]") :
			                    TEXT(""));
	}

	UE_LOG(LogBaamCard, Log, TEXT("총 %d 종류 / %d 장   (기대 %d : %s)"),
		Counts.Num(), Deck.Num(), ExpectedDeckSize,
		(Deck.Num() == ExpectedDeckSize) ? TEXT("일치") : TEXT("불일치 ← QuantityOfCard 확인"));

	if (NumUnimplemented > 0)
	{
		UE_LOG(LogBaamCard, Warning,
			TEXT("GA/GE 미지정 카드 %d 종이 덱에 포함되어 있습니다 (사용해도 효과 없음). "
			     "구현 전까지 QuantityOfCard 를 0 으로 두면 구현된 카드만으로 테스트할 수 있습니다."),
			NumUnimplemented);
	}

	if (NumDuplicateIds > 0)
	{
		// 중복되면 손패에서 카드를 제거할 때 엉뚱한 카드가 지워진다. 반드시 잡고 넘어갈 것.
		UE_LOG(LogBaamCard, Error,
			TEXT("InstanceId 중복 %d 건 — BuildDeck 의 NextInstanceId 카운터를 확인하세요."),
			NumDuplicateIds);
	}
#endif // !UE_BUILD_SHIPPING
}
