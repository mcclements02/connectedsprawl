// The Connected Sprawl - Deterministic occupant headroom tests.

#include "Vehicles/SprawlOccupantHeadroom.h"

#include "Vehicles/SprawlVehicleOccupantPlacement.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlOccupantHeadroomTest,
	"ConnectedSprawl.Vehicles.OccupantHeadroom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlOccupantHeadroomTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	constexpr float Height = 168.f;
	const float HeadAboveHips =
		Height * FSprawlOccupantHeadroom::SeatedHeadFraction;

	// A tall cabin: the seat needs no adjustment at all.
	FSprawlVehicleOccupantSeat Tall;
	Tall.bValid = true;
	Tall.ContainmentBounds = FBox(FVector(-100, -80, 20), FVector(100, 80, 160));
	Tall.Location = FVector(0, -20, 70);
	const FSprawlOccupantHeadroomFit TallFit =
		FSprawlOccupantHeadroom::Fit(Tall, Height);
	TestTrue(TEXT("A tall cabin accepts the seat unchanged"), TallFit.bValid);
	TestTrue(TEXT("No sink is applied in a tall cabin"),
		FMath::IsNearlyEqual(TallFit.SeatZ, 70.f));
	TestTrue(TEXT("No shrink is applied in a tall cabin"),
		FMath::IsNearlyEqual(TallFit.Scale, 1.f));

	// A low coupe: the head would breach the ceiling; sinking absorbs it.
	FSprawlVehicleOccupantSeat Low;
	Low.bValid = true;
	Low.ContainmentBounds = FBox(FVector(-100, -80, 16), FVector(100, 80, 88));
	Low.Location = FVector(0, -20, 44);
	const FSprawlOccupantHeadroomFit LowFit =
		FSprawlOccupantHeadroom::Fit(Low, Height);
	TestTrue(TEXT("A low cabin still seats the occupant"), LowFit.bValid);
	TestTrue(TEXT("The seat sinks toward the floor"), LowFit.SeatZ < 44.f);
	TestTrue(TEXT("The fitted head stays under the ceiling"),
		LowFit.SeatZ + HeadAboveHips * LowFit.Scale
			<= Low.ContainmentBounds.Max.Z + 0.1f);

	// An absurdly shallow body: no legal fit — the visual must be refused.
	FSprawlVehicleOccupantSeat Shallow;
	Shallow.bValid = true;
	Shallow.ContainmentBounds = FBox(FVector(-100, -80, 10), FVector(100, 80, 40));
	Shallow.Location = FVector(0, -20, 24);
	const FSprawlOccupantHeadroomFit ShallowFit =
		FSprawlOccupantHeadroom::Fit(Shallow, Height);
	TestFalse(TEXT("An uninhabitable body refuses the visual"),
		ShallowFit.bValid);

	// Invalid input never validates.
	FSprawlVehicleOccupantSeat Invalid;
	TestFalse(TEXT("An invalid seat never fits"),
		FSprawlOccupantHeadroom::Fit(Invalid, Height).bValid);
	return true;
}

#endif
