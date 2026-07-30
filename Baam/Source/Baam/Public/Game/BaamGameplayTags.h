#pragma once

#include "NativeGameplayTags.h"

// =====================================================================================
//  BANG! 게임태그 분류 (Prototype-Workflow.md §GameplayTag 분류)
// -------------------------------------------------------------------------------------
//  Team4Project 의 BaseGameplayTags 와 동일한 네이티브 태그 방식.
//    - 선언: 이 헤더에서 UE_DECLARE_GAMEPLAY_TAG_EXTERN
//    - 정의: BaamGameplayTags.cpp 에서 UE_DEFINE_GAMEPLAY_TAG(변수, "문자열")
//    - 사용: Bang::Card::Id::Bang.GetTag() 처럼 참조
//
//  ⚠️ 문자열이 곧 태그의 정체이며 DataAsset/GE/GA 전반에서 이 문자열로 매칭됩니다.
//     오타는 조용한 버그가 되므로 md 의 taxonomy 를 그대로 옮겼습니다.
// =====================================================================================

namespace Bang
{
	// ---------------------------------------------------------------------------------
	//  Card.* — 카드 정의용 태그. UBaamCardDef 가 카드 1종마다 부여.
	// ---------------------------------------------------------------------------------
	namespace Card
	{
		// Card.Type.* — 갈색(즉시 효과) / 파란색(장비, 공개 정보)
		namespace Type
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brown)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Blue)
		}

		// Card.Id.* — 카드 종류 식별자 (기본판 22종)
		namespace Id
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bang)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Missed)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Beer)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Saloon)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stagecoach)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WellsFargo)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(CatBalou)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Panic)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Duel)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(GeneralStore)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Indians)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gatling)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Jail)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dynamite)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Barrel)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mustang)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Scope)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Volcanic)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Schofield)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Remington)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RevCarabine)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Winchester)

			// ── BAAM 신규 카드 (Docs/Additional-Card-List.txt) ──
			//  기본판이 아니라 이 프로젝트에서 새로 만든 카드다(GDD §6.4).
			//  효과는 DT_BaamCard 의 OutcomeEffects 에 데이터로 적고,
			//  AbilityByCardId 에서 UGA_BaamCardEffects(범용 실행기)로 매핑한다.

			//  공격
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(QuickStrike)     // 속공
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ExposeWeakness)  // 약점 포착
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trap)            // 함정

			//  회복
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Prepare)         // 준비
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Rest)            // 휴식
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Brace)           // 대비

			//  장비(파란 카드)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(DwarfGloves)     // 드워프 장갑
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(FairyArmor)      // 요정 갑옷
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WitchCharm)      // 마녀의 부적
		}
		
		namespace Trait
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon)          // 무기(파란 카드, 1개만 장착)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(NeedsTargetSeat) // 대상 좌석 지정 필요
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reaction)        // 응답 창에서만 사용 (Missed! 등)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RangeLimited)    // 사거리(WeaponRange) 제약을 받음
		}
	}

	// ---------------------------------------------------------------------------------
	//  Ability.* — GA 의 Asset 태그. 카드별이 아니라 "메커니즘별" 1개 (md §1.1).
	//              개별 카드는 DataAsset 이 GA + 파라미터를 지정.
	// ---------------------------------------------------------------------------------
	namespace Ability
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bang)           // GA_Bang
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Missed)         // GA_Missed (응답 전용)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Heal)           // GA_Heal (Beer/Saloon)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(DrawN)          // GA_DrawN (역마차/웰스파고)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StealOrDiscard) // GA_StealOrDiscard (Panic/CatBalou)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(AreaBang)       // GA_AreaBang (개틀링/인디언)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Duel)           // GA_Duel
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Saloon)         // GA_Saloon
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GeneralStore)   // GA_GeneralStore
	}

	// ---------------------------------------------------------------------------------
	//  State.* — ASC 에 상주하는 상태 태그. GE(Infinite) 또는 룰 상태 머신이 부여.
	// ---------------------------------------------------------------------------------
	namespace State
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead)         // 사망 → 좌석 원에서 제외
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Jailed)       // 감옥. 턴 시작 뽑기 판정 대상
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(HasDynamite)  // 다이너마이트 보유

		// State.Turn.* — 현재 턴 진행 중 플레이어의 임시 상태
		namespace Turn
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Active)     // 지금 이 좌석의 턴
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BangPlayed) // 이번 턴 BANG! 사용함 (BangLimit 판정)
		}
	}

	// ---------------------------------------------------------------------------------
	//  Phase.* — GameState 의 서버 권위 페이즈 상태 머신 (GAS 아님, md §1.4).
	//            맵 이동 없이 Lobby → ... 로 태그만 전환.
	// ---------------------------------------------------------------------------------
	namespace Phase
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Lobby)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(TurnStart) // 감옥/다이너마이트 판정
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Draw)      // DrawCount 장 뽑기
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Play)      // 카드 사용
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Discard)   // 손패 한도(=Health)까지 버리기
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameOver)
	}

	// ---------------------------------------------------------------------------------
	//  Response.Allow.* — 응답 창(Resolution)에서 허용되는 응답 종류 (md §3).
	//                     FBangResolutionRequest.AllowedCards 에 담김.
	// ---------------------------------------------------------------------------------
	namespace Response
	{
		namespace Allow
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Missed)         // BANG!/개틀링에 Missed!
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bang)           // 인디언/결투에 Bang!
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PickRevealed)   // 잡화점: 공개된 카드 중 선택
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PickFromPlayer) // Panic!/Cat Balou: 대상에게서 선택
		}
	}

	// ---------------------------------------------------------------------------------
	//  Status.* — "다음 1회" 대기 상태 (Additional-Card-List 의 약점 포착 / 함정 / 준비 / 대비).
	//
	//    저장은 ABaamPlayerState::PendingStatuses 다 (태그 + 수치, 복제됨).
	//    ASC 루즈 태그를 쓰지 않는 이유:
	//      · 루즈 태그는 복제되지 않아 클라 UI 가 상태를 볼 수 없다
	//      · 태그만으로는 수치(약점 포착 +1 / +2)를 담을 수 없다
	//      · 검사 지점이 전부 서버 C++ 이라 GAS 태그 조회의 이점이 없다
	//
	//    ⚠️ 한 번 발동되면 소모된다. 중첩은 허용하지 않는다(이미 가진 상태를 주는 카드는 사용 거부).
	// ---------------------------------------------------------------------------------
	namespace Status
	{
		//  다음에 사용하는 카드 1장의 판정을 강제한다.
		namespace NextCard
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ForceSuccess)          // 준비(성공)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ForceCriticalSuccess)  // 준비(대성공)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ForceFailure)          // 함정(성공)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ForceCriticalFailure)  // 함정(대성공)
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(KeepCardUse)           // 대비 — 실패해도 사용한도 유지
		}

		//  다음 공격 1회에 적용된다.
		namespace NextAttack
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageBonus)           // 약점 포착 (수치 = 증가량)
		}
	}

	// ---------------------------------------------------------------------------------
	//  Resolution.* — 4단계 판정 결과 (GDD §4.1 / §9.3).
	//    서버가 판정한 뒤 GameplayEventData 에 실어 GA 로 넘긴다. GA 는 판정하지 않고 실행만 한다.
	// ---------------------------------------------------------------------------------
	namespace Resolution
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CriticalFailure)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Failure)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Success)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CriticalSuccess)
	}

	// ---------------------------------------------------------------------------------
	//  Role.* — 숨겨진 진영 역할. Sheriff 만 공개, 나머지는 사망 시 공개 (md §0/§1.3).
	// ---------------------------------------------------------------------------------
	namespace Role
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Sheriff)  // 보안관 (MaxHealth +1, 공개)
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Deputy)   // 부관
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Outlaw)   // 무법자
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Renegade) // 배신자
	}

	// ---------------------------------------------------------------------------------
	//  Character.Ability.* — 캐릭터 특수능력 16종 (md §T4).
	//                        각각 "규칙의 예외" → 패시브 GE/뽑기 오버라이드/이벤트 훅.
	// ---------------------------------------------------------------------------------
	namespace Character
	{
		namespace Ability
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PaulRegret)    // 거리 +1
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(RoseDoolan)    // 거리 -1
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(WillyTheKid)   // BANG! 무제한
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SlabTheKiller) // Missed! 2장 필요
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LuckyDuke)     // 뽑기 2장 중 선택
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Jourdonnais)   // 배럴 내장
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(CalamityJanet) // BANG!/Missed! 상호 사용
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BartCassidy)   // 피해 1당 1장 뽑기
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(ElGringo)      // 피해 준 자 손패 1장 획득
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SuzyLafayette) // 손패 0장이면 1장 뽑기
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SidKetchum)    // 손패 2장 버려 회복
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(VultureSam)    // 사망자 카드 전부 획득
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(JesseJones)    // 첫 장을 남의 손에서
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(PedroRamirez)  // 첫 장을 버린 패에서
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(KitCarlson)    // 3장 보고 2장 선택
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(BlackJack)     // 2번째 공개, 하트/다이아면 1장 더
		}
	}

	// ---------------------------------------------------------------------------------
	//  SetByCaller.* — GE 수치를 런타임에 주입할 때 쓰는 태그 (피해/회복 크기).
	// ---------------------------------------------------------------------------------
	namespace SetByCaller
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage) // GE_Damage 크기
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Heal)   // GE_Heal 크기
	}
}
