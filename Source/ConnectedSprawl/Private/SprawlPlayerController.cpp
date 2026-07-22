// The Connected Sprawl - Player controller.

#include "SprawlPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Missions/DecisionSubsystem.h"
#include "Missions/StrategicDecision.h"
#include "Founder/SprawlGameFlowSubsystem.h"
#include "Save/SprawlSaveSubsystem.h"
#include "Phone/PhoneSubsystem.h"
#include "UI/DecisionModalWidget.h"
#include "UI/SprawlDecisionModal.h"
#include "UI/SprawlEndGameModal.h"
#include "UI/SprawlCityMapWidget.h"
#include "UI/SprawlNativeHUD.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Vehicles/SprawlCar.h"
#include "Characters/ZarriCharacter.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

void ASprawlPlayerController::BeginPlay()
{
	Super::BeginPlay();
	EnsureUIInitialized();

	// GTA-style view limits: enough to scan rooftops or the tarmac, never a
	// full flip over the top. Applies on foot and behind the wheel.
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -52.f;
		PlayerCameraManager->ViewPitchMax = 28.f;
	}
}

void ASprawlPlayerController::ApplyTouchLook(const FVector2D& Delta)
{
	AddYawInput(Delta.X * TouchLookYawScale);
	AddPitchInput(Delta.Y * TouchLookPitchScale);
}

void ASprawlPlayerController::TouchJumpPressed()
{
	if (ACharacter* Character = Cast<ACharacter>(GetPawn()))
	{
		Character->Jump();
	}
}

void ASprawlPlayerController::TouchJumpReleased()
{
	if (ACharacter* Character = Cast<ACharacter>(GetPawn()))
	{
		Character->StopJumping();
	}
}

void ASprawlPlayerController::TouchSprintPressed()
{
	if (AZarriCharacter* Zarri = Cast<AZarriCharacter>(GetPawn()))
	{
		Zarri->SetSprinting(true);
	}
}

void ASprawlPlayerController::TouchSprintReleased()
{
	if (AZarriCharacter* Zarri = Cast<AZarriCharacter>(GetPawn()))
	{
		Zarri->SetSprinting(false);
	}
}

void ASprawlPlayerController::TouchThrottlePressed(float Direction)
{
	if (ASprawlCar* Car = Cast<ASprawlCar>(GetPawn()))
	{
		Car->SetTouchThrottle(Direction);
	}
}

void ASprawlPlayerController::TouchThrottleReleased()
{
	if (ASprawlCar* Car = Cast<ASprawlCar>(GetPawn()))
	{
		Car->SetTouchThrottle(0.f);
	}
}

bool ASprawlPlayerController::IsDrivingVehicle() const
{
	return Cast<ASprawlCar>(GetPawn()) != nullptr;
}

bool ASprawlPlayerController::IsCityMapOpen() const
{
	return CityMapWidget
		&& CityMapWidget->GetVisibility() != ESlateVisibility::Collapsed;
}

void ASprawlPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
		{
			Decisions->OnDecisionOffered.RemoveDynamic(
				this, &ASprawlPlayerController::HandleDecisionOffered);
		}
		if (USprawlGameFlowSubsystem* Flow =
			GI->GetSubsystem<USprawlGameFlowSubsystem>())
		{
			Flow->OnRunEnded.RemoveDynamic(
				this, &ASprawlPlayerController::HandleRunEnded);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ASprawlPlayerController::EnsureUIInitialized()
{
	// Called from BeginPlay AND SetupInputComponent: a BP subclass overriding
	// Event BeginPlay without a parent call must not silently lose the HUD,
	// the modal default, or the decision binding.
	if (bUIInitialized || !IsLocalController())
	{
		return;
	}
	bUIInitialized = true;

	// The native modal keeps the decision loop playable when no WBP override
	// was assigned in the BP subclass.
	if (!DecisionModalClass)
	{
		DecisionModalClass = USprawlDecisionModal::StaticClass();
	}
	if (!HUDWidgetClass)
	{
		HUDWidgetClass = USprawlNativeHUD::StaticClass();
	}
	if (!EndGameModalClass)
	{
		EndGameModalClass = USprawlEndGameModal::StaticClass();
	}

	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (USprawlNativeHUD* NativeHUD = Cast<USprawlNativeHUD>(HUDWidget))
		{
			NativeHUD->BuildUI();
		}
		// The currently authored WBP is an empty placeholder. Preserve any real
		// designer HUD, but recover to the complete native HUD when its tree has
		// no root and would otherwise render nothing.
		if (HUDWidget && !HUDWidget->GetRootWidget())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Sprawl] HUD class %s has no root; using native HUD"),
				*HUDWidgetClass->GetName());
			HUDWidgetClass = USprawlNativeHUD::StaticClass();
			HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
			if (USprawlNativeHUD* NativeHUD = Cast<USprawlNativeHUD>(HUDWidget))
			{
				NativeHUD->BuildUI();
			}
		}
		if (HUDWidget) HUDWidget->AddToViewport(0);
	}
	UE_LOG(LogTemp, Warning, TEXT("[Sprawl] UI init: HUD=%s modal=%s"),
		HUDWidgetClass ? *HUDWidgetClass->GetName() : TEXT("none"),
		DecisionModalClass ? *DecisionModalClass->GetName() : TEXT("none"));

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
		{
			Decisions->OnDecisionOffered.AddDynamic(this, &ASprawlPlayerController::HandleDecisionOffered);
		}
		if (USprawlGameFlowSubsystem* Flow =
			GI->GetSubsystem<USprawlGameFlowSubsystem>())
		{
			Flow->OnRunEnded.AddDynamic(
				this, &ASprawlPlayerController::HandleRunEnded);
			Flow->RefreshAfterLoad(true);
			if (Flow->IsGameOver())
			{
				HandleRunEnded(Flow->GetCurrentEndGameInfo());
			}
		}
	}
}

void ASprawlPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	EnsureUIInitialized();

	if (InputComponent)
	{
		// Recapture the freed cursor on a click without eating gameplay clicks.
		FInputKeyBinding& RecaptureBinding = InputComponent->BindKey(
			EKeys::LeftMouseButton, IE_Pressed,
			this, &ASprawlPlayerController::OnRecaptureClick);
		RecaptureBinding.bConsumeInput = false;

		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASprawlPlayerController::OnEscapePressed);
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ASprawlPlayerController::OnOnePressed);
		// Cmd/Ctrl + Shift + 0 also releases the cursor — same handler checks the modifiers.
		InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &ASprawlPlayerController::OnOnePressed);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ASprawlPlayerController::OnInteractPressed);
		InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ASprawlPlayerController::OnInteractPressed);
		FInputKeyBinding& MapBinding = InputComponent->BindKey(
			EKeys::M, IE_Pressed, this, &ASprawlPlayerController::OnMapPressed);
		MapBinding.bExecuteWhenPaused = true;
		// Keyboard/gamepad melee lives on the possessed Zarri input component,
		// above this controller in Unreal's input stack. This controller keeps
		// only the touch route and mouse-capture-aware left-click route.
		InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &ASprawlPlayerController::OnSavePressed);
		InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ASprawlPlayerController::OnLoadPressed);
	}
}

void ASprawlPlayerController::OnInteractPressed()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPhoneSubsystem* Phone = GI->GetSubsystem<UPhoneSubsystem>();
			Phone && Phone->TryAnswerRingingCall())
		{
			return;
		}
	}

	APawn* P = GetPawn();
	UE_LOG(LogTemp, Warning, TEXT("[Sprawl] Interact pressed, pawn=%s"),
		P ? *P->GetName() : TEXT("NULL"));

	if (ASprawlCar* Car = Cast<ASprawlCar>(P))
	{
		Car->RequestExit();
	}
	else if (AZarriCharacter* Zarri = Cast<AZarriCharacter>(P))
	{
		Zarri->TryContextInteraction();
	}
}

void ASprawlPlayerController::OnMapPressed()
{
	SetCityMapOpen(!IsCityMapOpen());
}

void ASprawlPlayerController::SetCityMapOpen(bool bOpen)
{
	if (!IsLocalController())
	{
		return;
	}
	if (bOpen && ((DecisionWidget && DecisionWidget->IsInViewport())
		|| (EndGameWidget && EndGameWidget->IsInViewport())))
	{
		return;
	}
	if (bOpen && !CityMapWidget)
	{
		CityMapWidget = CreateWidget<USprawlCityMapWidget>(
			this, USprawlCityMapWidget::StaticClass());
		if (CityMapWidget)
		{
			CityMapWidget->BuildUI();
			CityMapWidget->AddToViewport(80);
		}
	}
	if (!CityMapWidget)
	{
		return;
	}
	CityMapWidget->SetVisibility(
		bOpen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SetPause(bOpen);
	if (!bOpen)
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
	UE_LOG(LogTemp, Display, TEXT("[CityMap] %s"),
		bOpen ? TEXT("opened") : TEXT("closed"));
}

void ASprawlPlayerController::OnMeleePressed()
{
	if (AZarriCharacter* Zarri = Cast<AZarriCharacter>(GetPawn()))
	{
		Zarri->TryMeleeAttack();
	}
}

void ASprawlPlayerController::OnEscapePressed()
{
	if (IsCityMapOpen())
	{
		SetCityMapOpen(false);
		return;
	}
	// If a decision popup is currently active on screen, don't force quit on the first Esc press
	if ((DecisionWidget && DecisionWidget->IsInViewport()) ||
		(EndGameWidget && EndGameWidget->IsInViewport()))
	{
		return;
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USprawlSaveSubsystem* Saves = GI->GetSubsystem<USprawlSaveSubsystem>())
		{
			Saves->SaveProgress();
		}
	}

	// Quitting is a deliberate chord: Shift+Esc. A bare Esc saves and hands
	// the OS cursor back — the thing pause-menu reflexes actually want.
	const bool bShiftDown = IsInputKeyDown(EKeys::LeftShift)
		|| IsInputKeyDown(EKeys::RightShift);
	if (bShiftDown)
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
		return;
	}
	SetMouseCaptured(false);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<uint64>(0x5E5C), 4.f,
			FColor::White,
			TEXT("Saved · mouse freed — click the game to recapture · Shift+Esc quits"));
	}
	UE_LOG(LogTemp, Display, TEXT("[Sprawl] Esc: saved, cursor released"));
}

void ASprawlPlayerController::SetMouseCaptured(bool bCaptured)
{
	if (bCaptured)
	{
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		return;
	}
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ASprawlPlayerController::OnRecaptureClick()
{
	// Never attack or recapture underneath a modal.
	if ((DecisionWidget && DecisionWidget->IsInViewport())
		|| (EndGameWidget && EndGameWidget->IsInViewport())
		|| IsCityMapOpen())
	{
		return;
	}
	if (bShowMouseCursor)
	{
		SetMouseCaptured(true);
		return; // the click that returns to the game is never also an attack
	}
	OnMeleePressed();
}

void ASprawlPlayerController::OnSavePressed()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USprawlSaveSubsystem* Saves = GI->GetSubsystem<USprawlSaveSubsystem>())
		{
			Saves->SaveProgress();
		}
	}
}

void ASprawlPlayerController::OnLoadPressed()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USprawlSaveSubsystem* Saves = GI->GetSubsystem<USprawlSaveSubsystem>())
		{
			Saves->LoadProgressAndRestart();
		}
	}
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
	if (!Decision) return;
	SetCityMapOpen(false);
	if (!DecisionModalClass)
	{
		DecisionModalClass = USprawlDecisionModal::StaticClass();
	}

	UE_LOG(LogTemp, Warning, TEXT("[Sprawl] Presenting decision %s via %s"),
		*Decision->DecisionId.ToString(), *DecisionModalClass->GetName());

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

void ASprawlPlayerController::HandleRunEnded(const FSprawlEndGameInfo& Info)
{
	SetCityMapOpen(false);
	PendingEndGameInfo = Info;
	bHasPendingEndGame = true;
	if (UWorld* World = GetWorld())
	{
		// Decision resolution unpauses immediately after broadcasting. Present on
		// the next tick so a bailout shutdown cannot undo this modal's pause.
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(
				this, &ASprawlPlayerController::PresentPendingEndGame));
	}
}

void ASprawlPlayerController::PresentPendingEndGame()
{
	if (!bHasPendingEndGame || !IsLocalController())
	{
		return;
	}
	bHasPendingEndGame = false;

	if (!EndGameModalClass)
	{
		EndGameModalClass = USprawlEndGameModal::StaticClass();
	}
	if (EndGameWidget)
	{
		EndGameWidget->RemoveFromParent();
	}
	EndGameWidget = CreateWidget<UUserWidget>(this, EndGameModalClass);
	if (!EndGameWidget)
	{
		return;
	}
	if (USprawlEndGameModal* Modal = Cast<USprawlEndGameModal>(EndGameWidget))
	{
		Modal->PresentEndGame(PendingEndGameInfo);
	}
	EndGameWidget->AddToViewport(200);
	SetPause(true);
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
}
