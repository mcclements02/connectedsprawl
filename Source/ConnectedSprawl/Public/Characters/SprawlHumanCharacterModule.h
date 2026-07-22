// The Connected Sprawl - Reusable, replicated human-character contract.

#pragma once

#include "Characters/SprawlCharacterDeveloper.h"
#include "Characters/SprawlWardrobeModule.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SprawlHumanCharacterModule.generated.h"

class UAnimSequence;

/** Shared semantic actions for lightweight avatars and assembled MetaHumans. */
UENUM(BlueprintType)
enum class ESprawlHumanAction : uint8
{
	Stand,
	Walk,
	Run,
	Talk,
	Sit,
	Drive
};

/** Broad age direction used while authoring a unique MetaHuman identity. */
UENUM(BlueprintType)
enum class ESprawlHumanAgeBand : uint8
{
	YoungAdult,
	Adult,
	Mature,
	Elder
};

/** Visual presentation only; identity and pronouns remain designer-authored. */
UENUM(BlueprintType)
enum class ESprawlHumanPresentation : uint8
{
	Masculine,
	Feminine,
	Androgynous
};

UENUM(BlueprintType)
enum class ESprawlHumanBuild : uint8
{
	Lean,
	Average,
	Athletic,
	Broad
};

UENUM(BlueprintType)
enum class ESprawlHumanSkinTone : uint8
{
	Deep,
	Dark,
	MediumDeep,
	Medium,
	MediumLight,
	Light
};

UENUM(BlueprintType)
enum class ESprawlHumanHairTexture : uint8
{
	Bald,
	Straight,
	Wavy,
	Curly,
	Coily
};

/**
 * Compact runtime customization shared by every human actor.
 *
 * Zarri supplies the joints-only rig and action vocabulary, not a face to
 * clone. Each generated resident receives a stable, distinct authoring
 * direction plus a lightweight avatar fallback for the mobile crowd budget.
 */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlHumanCustomization
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Template")
	FName RigTemplateId = TEXT("Zarri_JointsOnly");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Identity")
	int32 Seed = 0;

	/** Authored Optimized/Low MetaHuman identity selected by the authority. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Identity")
	FName AppearanceId = TEXT("Zarri");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Appearance")
	ESprawlHumanAgeBand AgeBand = ESprawlHumanAgeBand::Adult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Appearance")
	ESprawlHumanPresentation Presentation = ESprawlHumanPresentation::Androgynous;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Appearance")
	ESprawlHumanBuild BodyBuild = ESprawlHumanBuild::Average;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Appearance")
	ESprawlHumanSkinTone SkinTone = ESprawlHumanSkinTone::MediumDeep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Appearance")
	ESprawlHumanHairTexture HairTexture = ESprawlHumanHairTexture::Curly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Appearance")
	ESprawlWardrobeStyle Wardrobe = ESprawlWardrobeStyle::Streetwear;

	/** Fully resolved layers/colors; replicated so every client dresses alike. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Appearance")
	FSprawlWardrobeOutfit Outfit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Appearance",
		meta=(ClampMin="150.0", ClampMax="205.0"))
	float HeightCm = 170.f;

	/** Legacy provenance only; ambient rendering never loads this toy avatar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|Legacy")
	FString LightweightAvatarVariant;

	/** Keeps assembled characters on the project's Optimized/Low mobile path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human|MetaHuman")
	bool bOptimizedMetaHuman = true;

	bool operator==(const FSprawlHumanCustomization& Other) const;
	bool operator!=(const FSprawlHumanCustomization& Other) const
	{
		return !(*this == Other);
	}
};

/** Small state payload suitable for normal ActorComponent replication. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlHumanRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Human")
	FName CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Human")
	FSprawlHumanCustomization Customization;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Human|Action")
	ESprawlHumanAction Action = ESprawlHumanAction::Stand;

	/** Held talk/sit/drive poses are not replaced by automatic speed updates. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Human|Action")
	bool bActionHeld = false;

	/** Monotonic state revision lets Blueprint consumers reject stale events. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Human|Replication")
	int32 Revision = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSprawlHumanRuntimeStateChanged, FSprawlHumanRuntimeState, RuntimeState);

/**
 * Replication-safe human behavior/customization module.
 *
 * It deliberately owns compact intent rather than body components or model
 * loads. Zarri, pedestrians, mission NPCs, and future Mass proxies can consume
 * the same state while choosing the visual tier appropriate to their budget.
 */
UCLASS(ClassGroup=(Sprawl), meta=(BlueprintSpawnableComponent))
class CONNECTEDSPRAWL_API USprawlHumanCharacterModule : public UActorComponent
{
	GENERATED_BODY()

public:
	USprawlHumanCharacterModule();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Generate a stable, varied Zarri-compatible identity from an existing profile. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Human Character")
	bool ConfigureFromProfile(const FSprawlCharacterProfile& Profile);

	/** Configure from a named-character asset without loading its MetaHuman class. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Human Character")
	bool ConfigureFromDefinition(USprawlCharacterDefinition* Definition);

	/** Exact hero defaults; other people only share this rig/action contract. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Human Character")
	bool ConfigureZarri(int32 Seed = 20050101);

	/** Authority-owned edit. Replicates when the owning actor is replicated. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Human Character")
	bool SetCustomization(
		const FSprawlHumanCustomization& Customization, FString& OutError);

	/**
	 * Set an explicit action. Hold talk/sit/drive until ClearHeldAction when a
	 * script, interaction, or vehicle transition owns the pose.
	 */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Human Character")
	bool SetAction(ESprawlHumanAction NewAction, bool bHoldAction = false);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Human Character")
	bool ClearHeldAction();

	/** Derive stand/walk/run/talk/sit/drive without breaking an explicitly held pose. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Human Character")
	ESprawlHumanAction UpdateActionFromMovement(
		float GroundSpeed, bool bTalking, bool bSeated,
		bool bDriving, bool bForceRun = false);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Human Character")
	const FSprawlHumanRuntimeState& GetRuntimeState() const { return RuntimeState; }

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Human Character")
	const FSprawlCharacterProfile& GetSourceProfile() const { return SourceProfile; }

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Human Character")
	bool IsConfigured() const { return !RuntimeState.CharacterId.IsNone(); }

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Human Character")
	bool IsActionHeld() const { return RuntimeState.bActionHeld; }

	/** Pure helpers are shared by authoring, runtime actors, and automation tests. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Human Character")
	static FSprawlHumanCustomization DevelopDiverseCustomization(
		const FSprawlCharacterProfile& Profile);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Human Character")
	static FSprawlHumanCustomization CreateZarriCustomization(int32 Seed = 20050101);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Human Character")
	static ESprawlHumanAction ResolveAction(
		float GroundSpeed, bool bTalking, bool bSeated,
		bool bDriving, bool bForceRun = false);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Human Character")
	static bool ValidateCustomization(
		const FSprawlHumanCustomization& Customization, FString& OutError);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Human Character")
	static FString DescribeCustomization(
		const FSprawlHumanCustomization& Customization);

	/** Imported-avatar suffix; Drive intentionally reuses the seated pose. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Human Character")
	static FName FallbackAnimationSuffix(ESprawlHumanAction Action);

	/** Soft named-character animation binding; never loads the asset. */
	static TSoftObjectPtr<UAnimSequence> AnimationForAction(
		const USprawlCharacterDefinition* Definition, ESprawlHumanAction Action);

	UPROPERTY(BlueprintAssignable, Category="Connected Sprawl|Human Character")
	FSprawlHumanRuntimeStateChanged OnRuntimeStateChanged;

protected:
	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleInstanceOnly,
		BlueprintReadOnly, Category="Human")
	FSprawlHumanRuntimeState RuntimeState;

	/** Full authoring text stays local and never bloats the replicated payload. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="Human")
	FSprawlCharacterProfile SourceProfile;

	UFUNCTION()
	void OnRep_RuntimeState();

private:
	bool CanMutateState() const;
	void CommitState(FSprawlHumanRuntimeState NewState);
};
