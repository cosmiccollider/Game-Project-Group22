// Fill out your copyright notice in the Description page of Project Settings.


#include "DefaultCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "UI/OverlayUserWidget.h"
#include "UI/PauseMenuUserWidget.h"
#include "UObject/ConstructorHelpers.h"

#define HOLD_DISTANCE 200
#define MAIN_MENU_MAP "MainMenuMap"
#define ROTATION_SENSITIVITY 0.5f
#define TRACE_DISTANCE 400

// ==================== Character ====================

// Sets default values
ADefaultCharacter::ADefaultCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	Camera->bUsePawnControlRotation = true;

	// Create a physics constraint
	PhysicsConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("PhysicsConstraintComponent"));

	// Create a physics handle
	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandleComponent"));

	// Set Default Mapping Context and Input Action default assets
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultMappingContextFinder(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	if (DefaultMappingContextFinder.Succeeded())
	{
		DefaultMappingContext = DefaultMappingContextFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchActionFinder(TEXT("/Game/Input/Actions/IA_Crouch.IA_Crouch"));
	if (CrouchActionFinder.Succeeded())
	{
		CrouchAction = CrouchActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionFinder(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	if (JumpActionFinder.Succeeded())
	{
		JumpAction = JumpActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	if (LookActionFinder.Succeeded())
	{
		LookAction = LookActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	if (MoveActionFinder.Succeeded())
	{
		MoveAction = MoveActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> PauseActionFinder(TEXT("/Game/Input/Actions/IA_Pause.IA_Pause"));
	if (PauseActionFinder.Succeeded())
	{
		PauseAction = PauseActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> PrimaryActionFinder(TEXT("/Game/Input/Actions/IA_Primary.IA_Primary"));
	if (PrimaryActionFinder.Succeeded())
	{
		PrimaryAction = PrimaryActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> SecondaryActionFinder(TEXT("/Game/Input/Actions/IA_Secondary.IA_Secondary"));
	if (SecondaryActionFinder.Succeeded())
	{
		SecondaryAction = SecondaryActionFinder.Object;
	}

	// Set class variables
	bCanLook = true;
	bInPauseMenu = false;
	bIsGrabbing = false;
	bIsRotating = false;

	// Set Overlay Widget class
	static ConstructorHelpers::FClassFinder<UOverlayUserWidget> OverlayFinder(TEXT("/Game/UI/WBP_Overlay.WBP_Overlay_C"));
	if (OverlayFinder.Succeeded())
	{
		OverlayClass = OverlayFinder.Class;
	}

	// Set Pause Menu Widget class
	static ConstructorHelpers::FClassFinder<UPauseMenuUserWidget> PauseMenuFinder(TEXT("/Game/UI/WBP_PauseMenu.WBP_PauseMenu_C"));
	if (PauseMenuFinder.Succeeded())
	{
		PauseMenuClass = PauseMenuFinder.Class;
	}
}

// Called every frame
void ADefaultCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsGrabbing)
	{
		MoveObject();

		if (bIsRotating)
		{
			RotateObject();
		}
	}
}

// Called to bind functionality to input
void ADefaultCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Crouch
		Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ADefaultCharacter::ToggleCrouch);

		// Jump
		Input->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Look
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADefaultCharacter::Look);

		// Move
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADefaultCharacter::Move);

		// Pause
		Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ADefaultCharacter::Pause);

		// Primary
		Input->BindAction(PrimaryAction, ETriggerEvent::Started, this, &ADefaultCharacter::StartPrimary);
		Input->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &ADefaultCharacter::StopPrimary);

		// Secondary
		Input->BindAction(SecondaryAction, ETriggerEvent::Started, this, &ADefaultCharacter::StartSecondary);
		Input->BindAction(SecondaryAction, ETriggerEvent::Completed, this, &ADefaultCharacter::StopSecondary);
	}
}

// Called when the game starts or when spawned
void ADefaultCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		// Add Overlay
		if (OverlayClass)
		{
			Overlay = CreateWidget<UOverlayUserWidget>(PC, OverlayClass);
			Overlay->AddToViewport();

			// Only add the reticle if we are not in the main menu
			if (UGameplayStatics::GetCurrentLevelName(GetWorld()) != MAIN_MENU_MAP)
			{
				Overlay->ShowReticle(true);
			}
		}
	}
}

// Get Overlay User Widget
UOverlayUserWidget* ADefaultCharacter::GetOverlay()
{
	return Overlay;
}

// ==================== Input Actions ====================

// Called to toggle crouch input
void ADefaultCharacter::ToggleCrouch()
{
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
	}
	else
	{
		GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
		Crouch();
	}
}

// Called to trigger look input
void ADefaultCharacter::Look(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller && bCanLook)
	{
		// Add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// Called to trigger move input
void ADefaultCharacter::Move(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		// Add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

// Called to open or close pause menu widget
void ADefaultCharacter::Pause()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (PauseMenuClass && !bInPauseMenu)
	{
		// Center mouse so the player doesn't have to spend time finding the mouse when it becomes visible
		SetMouseCenter();

		// Create Pause Menu widget and add it to the screen
		PauseMenu = CreateWidget<UPauseMenuUserWidget>(PC, PauseMenuClass);
		PauseMenu->AddToViewport();

		// Set input mode to Game and UI so the player can use mouse or keyboard inputs, then show the mouse cursor
		FInputModeGameAndUI InputMode;
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);

		// Set game to paused
		UGameplayStatics::SetGamePaused(GetWorld(), true);
		bInPauseMenu = true;
	}
	else if (PauseMenu)
	{
		// Remove Pause Menu from the screen
		PauseMenu->RemoveFromParent();
		PauseMenu = nullptr;

		// Set input mode to Game and hide the mouse cursor
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(false);

		// Set game to unpaused
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		bInPauseMenu = false;
	}
}

// Called to start primary mouse input
void ADefaultCharacter::StartPrimary()
{
	StartGrab();
}

// Called to stop primary mouse input
void ADefaultCharacter::StopPrimary()
{
	if (bIsGrabbing)
	{
		StopGrab();
	}
}

// Called to start secondary mouse input
void ADefaultCharacter::StartSecondary()
{
	if (bIsGrabbing)
	{
		StartRotation();
	}
}

// Called to stop secondary mouse input
void ADefaultCharacter::StopSecondary()
{
	if (bIsRotating)
	{
		StopRotation();
	}
}

// ==================== Physics ====================

void ADefaultCharacter::SetGravity(bool bEnabled)
{
	if (bEnabled)
	{
		GetCharacterMovement()->GravityScale = 1;
	}
	else
	{
		GetCharacterMovement()->GravityScale = 0;
	}
}

// Called to start grabbing an object
void ADefaultCharacter::StartGrab()
{
	if (Camera)
	{
		// Set the distance to trace, which defines the delta from the start point (current location of the camera) to the end point
		FVector TraceDistance = Camera->GetComponentRotation().Vector() * TRACE_DISTANCE;
		FVector Start = Camera->GetComponentLocation();
		FVector End = Start + TraceDistance;

		// Create a line trace and check that it finds a hit result
		FHitResult OutHit;
		bool LineTrace = UKismetSystemLibrary::LineTraceSingle(GetWorld(), Start, End, ETraceTypeQuery(), false, TArray<AActor*>(), EDrawDebugTrace::ForDuration, OutHit, true);
		if (LineTrace && PhysicsHandle)
		{
			// Initialize a zeroed FRotator and grab the hit result with that rotation
			// A zeroed FRotator is necessary for the rotation function because the default FRotator() constructor does not initialize a value of 0
			// If it isn't initilized at 0, the rotation will be offset when starting to rotating, since rotating operates on a delta with the center of the screen
			FRotator Rotation = FRotator(0);
			PhysicsHandle->GrabComponentAtLocationWithRotation(OutHit.GetComponent(), FName(), OutHit.GetComponent()->GetComponentLocation(), Rotation);
			bIsGrabbing = true;
		}
	}
}

// Called to stop grabbing an object
void ADefaultCharacter::StopGrab()
{
	// Release object and disable grabbing
	PhysicsHandle->ReleaseComponent();
	bIsGrabbing = false;
}

// Called to update the location of a grabbed object
void ADefaultCharacter::MoveObject()
{
	// Get location as a distance from the camera based on current rotation
	FVector HoldDistance = Camera->GetComponentRotation().Vector() * HOLD_DISTANCE;
	FVector Location = Camera->GetComponentLocation() + HoldDistance;

	// Set location of object
	PhysicsHandle->SetTargetLocation(Location);
}

// Called to start rotating an object
void ADefaultCharacter::StartRotation()
{
	// Center mouse to allow for the most possible rotation in any direction, since the mouse stops at the edge of the screen
	SetMouseCenter();

	// Disable looking and enable rotating
	bCanLook = false;
	bIsRotating = true;
}

// Called to stop rotating an object
void ADefaultCharacter::StopRotation()
{
	// Disable rotating and enable looking
	bIsRotating = false;
	bCanLook = true;
}

// Called to update the rotation of a grabbed object
void ADefaultCharacter::RotateObject()
{
	// Get the current mouse position
	float MouseX;
	float MouseY;
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PC->GetMousePosition(MouseX, MouseY);

	// Set pitch and yaw according to the difference between current mouse position and viewport center
	FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld());
	float Pitch = (MouseY - (ViewportSize.Y / 2.0f)) * ROTATION_SENSITIVITY;
	float Yaw = (MouseX - (ViewportSize.X / 2.0f)) * ROTATION_SENSITIVITY;

	// Set new rotation of object based on pitch and yaw
	PhysicsHandle->SetTargetRotation(UKismetMathLibrary::MakeRotator(0.0f, Pitch, Yaw));
}

// ==================== Helpers ====================

// Called to set the mouse location to the center of the screen
void ADefaultCharacter::SetMouseCenter()
{
	// Get player controller and size of screen
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld());

	// Set mouse location
	PC->SetMouseLocation(UKismetMathLibrary::FTrunc(ViewportSize.X / 2.0f), UKismetMathLibrary::FTrunc(ViewportSize.Y / 2.0f));
}
