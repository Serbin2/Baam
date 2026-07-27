#include "Game/BaamGameMode.h"
#include "Game/BaamGameplayTags.h"
#include "Player/BaamCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

TArray<FGameplayTag> ABaamGameMode::BuildRolePool(int32 NumPlayers)
{
	using namespace Bang::Role;

	TArray<FGameplayTag> Pool;

	// 공통: 보안관 1, 배신자 1.
	Pool.Add(Sheriff.GetTag());
	Pool.Add(Renegade.GetTag());

	// 무법자: 4~5인 2명 / 6~7인 3명.
	const int32 NumOutlaws = (NumPlayers <= 5) ? 2 : 3;
	for (int32 i = 0; i < NumOutlaws; ++i)
	{
		Pool.Add(Outlaw.GetTag());
	}

	// 부관: 남는 자리를 채운다 → 4인 0 / 5·6인 1 / 7인 2.
	const int32 NumDeputies = NumPlayers - Pool.Num();
	for (int32 i = 0; i < NumDeputies; ++i)
	{
		Pool.Add(Deputy.GetTag());
	}

	return Pool;
}

void ABaamGameMode::CollectPlayers(TArray<APlayerController*>& OutPlayers) const
{
	OutPlayers.Reset();

	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				OutPlayers.Add(PC);
			}
		}
	}
}

void ABaamGameMode::AssignRoles()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<APlayerController*> Players;
	CollectPlayers(Players);

	const int32 Num = Players.Num();
	if (Num < MinPlayers || Num > MaxPlayers)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Bang] AssignRoles: 접속 인원 %d 명은 지원 범위(%d~%d) 밖 — 배정 취소."),
			Num, MinPlayers, MaxPlayers);
		return;
	}

	TArray<FGameplayTag> Pool = BuildRolePool(Num);
	if (Pool.Num() != Num)
	{
		// 인원수와 풀 크기가 어긋나면(로직 오류) 잘못된 배정을 막는다.
		UE_LOG(LogTemp, Error, TEXT("[Bang] AssignRoles: 역할 풀(%d)과 인원(%d) 불일치."),
			Pool.Num(), Num);
		return;
	}

	// Fisher-Yates 셔플 (뒤에서부터 랜덤 교환).
	for (int32 i = Pool.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Pool.Swap(i, j);
	}

	// 셔플된 풀에서 한 명씩 태그를 빼서 배정.
	// SetCharacterTag 가 캐릭터의 ASC 에 DT Row(스탯 GE/패시브/어빌리티)를 부여한다.
	for (int32 i = 0; i < Num; ++i)
	{
		APlayerController* PC = Players[i];
		ABaamCharacter* Char = PC ? Cast<ABaamCharacter>(PC->GetPawn()) : nullptr;
		if (!Char)
		{
			// 아직 폰이 스폰되지 않은 플레이어 — 스킵 (로그로 남겨 추적).
			UE_LOG(LogTemp, Warning, TEXT("[Bang] AssignRoles: %s 의 캐릭터 없음 — 배정 스킵."),
				PC ? *PC->GetName() : TEXT("null"));
			continue;
		}

		const FGameplayTag RoleTag = Pool[i];
		Char->SetCharacterTag(RoleTag);

		UE_LOG(LogTemp, Log, TEXT("[Bang] AssignRoles: %s → %s"),
			*PC->GetName(), *RoleTag.ToString());
	}
}
