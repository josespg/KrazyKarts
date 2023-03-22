// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;

	// To distinguish moves, use a timestamp like this, time where the move started.
	UPROPERTY()
	float Time;

	inline bool IsValid() const
	{
		return FMath::Abs(Throttle) <= 1.0f &&
			FMath::Abs(SteeringThrow) <= 1.0f;
	};
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SimulateMove(const FGoKartMove& Move);

	void SetVelocity(const FVector& _Velocity) { Velocity = _Velocity; };
	const FVector& GetVelocity() const { return Velocity; };

	void SetThrottle(const float _Throttle) { Throttle = _Throttle; };
	float GetThrottle() const { return Throttle; }

	void SetSteeringThrow(const float _SteeringThrow) { SteeringThrow = _SteeringThrow; };
	float GetSteeringThrow() const { return SteeringThrow; }

	const FGoKartMove& GetLastMove() const { return LastMove; };

private:

	FGoKartMove CreateMove(float DeltaTime) const;

	FVector GetAirResistance() const;
	FVector GetRollingResistance() const;
	void UpdateLocationFromVelocity(float DeltaTime);
	void ApplyRotation(float DeltaTime, float SteeringThrow);

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

	FVector Velocity;

	float Throttle;
	float SteeringThrow;

	FGoKartMove LastMove;
};
