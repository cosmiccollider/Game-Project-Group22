// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "PhysicsStaticMeshActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_API APhysicsStaticMeshActor : public AStaticMeshActor
{
	GENERATED_BODY()
	
public:
	UFUNCTION()
		void EnableGravity();

	UFUNCTION()
		void DisableGravity();
};