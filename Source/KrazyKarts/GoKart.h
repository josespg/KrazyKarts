// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	FVector GetAirResistance() const;
	FVector GetRollingResistance() const;
	void UpdateLocationFromVelocity(float DeltaTime);
	void ApplyRotation(float DeltaTime);

	// The mass of the car (kg).
	UPROPERTY(EditAnywhere)
	float Mass = 1000.0f;

	// The force applied to the car when the throttle is full down (N).
	UPROPERTY(EditAnywhere)
	//float MaxDrivingForce = 1000000.0f;
	float MaxDrivingForce = 10000.0f;

	// The number of degrees rotated per second at full control throw (degrees/s).
	UPROPERTY(EditAnywhere)
	float MaxDegreesPerSecond = 90.0f;
	
	// Minimum radius of the car turning circle at full lock (m).
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10.0f;

	// Higher means more drag (Kg*m ?).
	// AirResistance = -Speed^2 * DragCoefficient
	// AirResistance / Speed^2 = DragCoefficient
	// 10000 / 25^2 = DragCoefficient = 16
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16.0f;//0.16

	// Higher means more rolling resistance
	// From Rolling Resistance wikipedia page coefficient examples
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.15f;//15.0f;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveForward(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveRight(float Value);

	FVector Velocity;

	float Throttle;
	float SteeringThrow;
};
