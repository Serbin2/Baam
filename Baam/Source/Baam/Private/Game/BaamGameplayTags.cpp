#include "Game/BaamGameplayTags.h"

// =====================================================================================
//  BANG! 게임태그 정의 (선언은 Public/Game/BaamGameplayTags.h)
//  변수 경로(Bang::Card::Id::Bang)와 문자열("Card.Id.Bang")을 1:1로 유지한다.
// =====================================================================================

namespace Bang
{
	namespace Card
	{
		namespace Type
		{
			UE_DEFINE_GAMEPLAY_TAG(Brown, "Card.Type.Brown")
			UE_DEFINE_GAMEPLAY_TAG(Blue,  "Card.Type.Blue")
		}

		namespace Id
		{
			UE_DEFINE_GAMEPLAY_TAG(Bang,         "Card.Id.Bang")
			UE_DEFINE_GAMEPLAY_TAG(Missed,       "Card.Id.Missed")
			UE_DEFINE_GAMEPLAY_TAG(Beer,         "Card.Id.Beer")
			UE_DEFINE_GAMEPLAY_TAG(Saloon,       "Card.Id.Saloon")
			UE_DEFINE_GAMEPLAY_TAG(Stagecoach,   "Card.Id.Stagecoach")
			UE_DEFINE_GAMEPLAY_TAG(WellsFargo,   "Card.Id.WellsFargo")
			UE_DEFINE_GAMEPLAY_TAG(CatBalou,     "Card.Id.CatBalou")
			UE_DEFINE_GAMEPLAY_TAG(Panic,        "Card.Id.Panic")
			UE_DEFINE_GAMEPLAY_TAG(Duel,         "Card.Id.Duel")
			UE_DEFINE_GAMEPLAY_TAG(GeneralStore, "Card.Id.GeneralStore")
			UE_DEFINE_GAMEPLAY_TAG(Indians,      "Card.Id.Indians")
			UE_DEFINE_GAMEPLAY_TAG(Gatling,      "Card.Id.Gatling")
			UE_DEFINE_GAMEPLAY_TAG(Jail,         "Card.Id.Jail")
			UE_DEFINE_GAMEPLAY_TAG(Dynamite,     "Card.Id.Dynamite")
			UE_DEFINE_GAMEPLAY_TAG(Barrel,       "Card.Id.Barrel")
			UE_DEFINE_GAMEPLAY_TAG(Mustang,      "Card.Id.Mustang")
			UE_DEFINE_GAMEPLAY_TAG(Scope,        "Card.Id.Scope")
			UE_DEFINE_GAMEPLAY_TAG(Volcanic,     "Card.Id.Volcanic")
			UE_DEFINE_GAMEPLAY_TAG(Schofield,    "Card.Id.Schofield")
			UE_DEFINE_GAMEPLAY_TAG(Remington,    "Card.Id.Remington")
			UE_DEFINE_GAMEPLAY_TAG(RevCarabine,  "Card.Id.RevCarabine")
			UE_DEFINE_GAMEPLAY_TAG(Winchester,   "Card.Id.Winchester")
		}

		namespace Trait
		{
			UE_DEFINE_GAMEPLAY_TAG(Weapon,          "Card.Trait.Weapon")
			UE_DEFINE_GAMEPLAY_TAG(NeedsTargetSeat, "Card.Trait.NeedsTargetSeat")
			UE_DEFINE_GAMEPLAY_TAG(Reaction,        "Card.Trait.Reaction")
			UE_DEFINE_GAMEPLAY_TAG(RangeLimited,    "Card.Trait.RangeLimited")
		}
	}

	namespace Ability
	{
		UE_DEFINE_GAMEPLAY_TAG(Bang,           "Ability.Bang")
		UE_DEFINE_GAMEPLAY_TAG(Missed,         "Ability.Missed")
		UE_DEFINE_GAMEPLAY_TAG(Heal,           "Ability.Heal")
		UE_DEFINE_GAMEPLAY_TAG(DrawN,          "Ability.DrawN")
		UE_DEFINE_GAMEPLAY_TAG(StealOrDiscard, "Ability.StealOrDiscard")
		UE_DEFINE_GAMEPLAY_TAG(AreaBang,       "Ability.AreaBang")
		UE_DEFINE_GAMEPLAY_TAG(Duel,           "Ability.Duel")
		UE_DEFINE_GAMEPLAY_TAG(Saloon,         "Ability.Saloon")
		UE_DEFINE_GAMEPLAY_TAG(GeneralStore,   "Ability.GeneralStore")
	}

	namespace State
	{
		UE_DEFINE_GAMEPLAY_TAG(Dead,        "State.Dead")
		UE_DEFINE_GAMEPLAY_TAG(Jailed,      "State.Jailed")
		UE_DEFINE_GAMEPLAY_TAG(HasDynamite, "State.HasDynamite")

		namespace Turn
		{
			UE_DEFINE_GAMEPLAY_TAG(Active,     "State.Turn.Active")
			UE_DEFINE_GAMEPLAY_TAG(BangPlayed, "State.Turn.BangPlayed")
		}
	}

	namespace Phase
	{
		UE_DEFINE_GAMEPLAY_TAG(Lobby,     "Phase.Lobby")
		UE_DEFINE_GAMEPLAY_TAG(TurnStart, "Phase.TurnStart")
		UE_DEFINE_GAMEPLAY_TAG(Draw,      "Phase.Draw")
		UE_DEFINE_GAMEPLAY_TAG(Play,      "Phase.Play")
		UE_DEFINE_GAMEPLAY_TAG(Discard,   "Phase.Discard")
		UE_DEFINE_GAMEPLAY_TAG(GameOver,  "Phase.GameOver")
	}

	namespace Response
	{
		namespace Allow
		{
			UE_DEFINE_GAMEPLAY_TAG(Missed,         "Response.Allow.Missed")
			UE_DEFINE_GAMEPLAY_TAG(Bang,           "Response.Allow.Bang")
			UE_DEFINE_GAMEPLAY_TAG(PickRevealed,   "Response.Allow.PickRevealed")
			UE_DEFINE_GAMEPLAY_TAG(PickFromPlayer, "Response.Allow.PickFromPlayer")
		}
	}

	namespace Resolution
	{
		UE_DEFINE_GAMEPLAY_TAG(CriticalFailure, "Resolution.CriticalFailure")
		UE_DEFINE_GAMEPLAY_TAG(Failure,         "Resolution.Failure")
		UE_DEFINE_GAMEPLAY_TAG(Success,         "Resolution.Success")
		UE_DEFINE_GAMEPLAY_TAG(CriticalSuccess, "Resolution.CriticalSuccess")
	}

	namespace Role
	{
		UE_DEFINE_GAMEPLAY_TAG(Sheriff,  "Role.Sheriff")
		UE_DEFINE_GAMEPLAY_TAG(Deputy,   "Role.Deputy")
		UE_DEFINE_GAMEPLAY_TAG(Outlaw,   "Role.Outlaw")
		UE_DEFINE_GAMEPLAY_TAG(Renegade, "Role.Renegade")
	}

	namespace Character
	{
		namespace Ability
		{
			UE_DEFINE_GAMEPLAY_TAG(PaulRegret,    "Character.Ability.PaulRegret")
			UE_DEFINE_GAMEPLAY_TAG(RoseDoolan,    "Character.Ability.RoseDoolan")
			UE_DEFINE_GAMEPLAY_TAG(WillyTheKid,   "Character.Ability.WillyTheKid")
			UE_DEFINE_GAMEPLAY_TAG(SlabTheKiller, "Character.Ability.SlabTheKiller")
			UE_DEFINE_GAMEPLAY_TAG(LuckyDuke,     "Character.Ability.LuckyDuke")
			UE_DEFINE_GAMEPLAY_TAG(Jourdonnais,   "Character.Ability.Jourdonnais")
			UE_DEFINE_GAMEPLAY_TAG(CalamityJanet, "Character.Ability.CalamityJanet")
			UE_DEFINE_GAMEPLAY_TAG(BartCassidy,   "Character.Ability.BartCassidy")
			UE_DEFINE_GAMEPLAY_TAG(ElGringo,      "Character.Ability.ElGringo")
			UE_DEFINE_GAMEPLAY_TAG(SuzyLafayette, "Character.Ability.SuzyLafayette")
			UE_DEFINE_GAMEPLAY_TAG(SidKetchum,    "Character.Ability.SidKetchum")
			UE_DEFINE_GAMEPLAY_TAG(VultureSam,    "Character.Ability.VultureSam")
			UE_DEFINE_GAMEPLAY_TAG(JesseJones,    "Character.Ability.JesseJones")
			UE_DEFINE_GAMEPLAY_TAG(PedroRamirez,  "Character.Ability.PedroRamirez")
			UE_DEFINE_GAMEPLAY_TAG(KitCarlson,    "Character.Ability.KitCarlson")
			UE_DEFINE_GAMEPLAY_TAG(BlackJack,     "Character.Ability.BlackJack")
		}
	}

	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG(Damage, "SetByCaller.Damage")
		UE_DEFINE_GAMEPLAY_TAG(Heal,   "SetByCaller.Heal")
	}
}
