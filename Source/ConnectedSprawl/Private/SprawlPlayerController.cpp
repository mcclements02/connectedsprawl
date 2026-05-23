// The Connected Sprawl - Player controller.

#include "SprawlPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Missions/DecisionSubsystem.h"
#include "Missions/StrategicDecision.h"
#include "UI/DecisionModalWidget.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Components/InputComponent.h"
#include "Vehicles/SprawlCar.h"
#include "Characters/ZarriCharacter.h"
#include "GameFramework/Pawn.h"

void ASprawlPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget) HUDWidget->AddToViewport(0);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
		{
			Decisions->OnDecisionOffered.AddDynamic(this, &ASprawlPlayerController::HandleDecisionOffered);
		}
	}
}

void ASprawlPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASprawlPlayerController::OnEscapePressed);
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ASprawlPlayerController::OnOnePressed);
		// Cmd/Ctrl + Shift + 0 also releases the cursor — same handler checks the modifiers.
		InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &ASprawlPlayerController::OnOnePressed);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ASprawlPlayerController::OnInteractPressed);
		InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ASprawlPlayerController::OnInteractPressed);
	}
}

void ASprawlPlayerController::OnInteractPressed()
{
	APawn* P = GetPawn();
	UE_LOG(LogTemp, Warning, TEXT("[Sprawl] Interact pressed, pawn=%s"),
		P ? *P->GetName() : TEXT("NULL"));

	if (ASprawlCar* Car = Cast<ASprawlCar>(P))
	{
		Car->RequestExit();
	}
	else if (AZarriCharacter* Zarri = Cast<AZarriCharacter>(P))
	{
		Zarri->EnterNearbyVehicle();
	}
}

void ASprawlPlayerController::OnEscapePressed()
{
	// If a decision popup is currently active on screen, don't force quit on the first Esc press
	if (DecisionWidget && DecisionWidget->IsInViewport())
	{
		return;
	}

	UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
}

void ASprawlPlayerController::OnOnePressed()
{
	const bool bShiftDown = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
	const bool bCmdDown = IsInputKeyDown(EKeys::LeftCommand) || IsInputKeyDown(EKeys::RightCommand) ||
	                      IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);

	if (bShiftDown && bCmdDown)
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
}

void ASprawlPlayerController::HandleDecisionOffered(UStrategicDecision* Decision)
{
	if (!Decision || !DecisionModalClass) return;

	DecisionWidget = CreateWidget<UUserWidget>(this, DecisionModalClass);
	if (!DecisionWidget) return;

	if (UDecisionModalWidget* Modal = Cast<UDecisionModalWidget>(DecisionWidget))
	{
		Modal->PresentDecision(Decision);
	}
	DecisionWidget->AddToViewport(100);
	SetPause(true);
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
}
