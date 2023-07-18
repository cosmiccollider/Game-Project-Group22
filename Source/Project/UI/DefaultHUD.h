// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DefaultHUD.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_API ADefaultHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override;
};