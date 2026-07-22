// The Connected Sprawl - Deterministic directed-traffic rule tests.

#include "AI/SprawlTrafficLaneDiscipline.h"
#include "AI/SprawlTrafficRoute.h"
#include "Vehicles/SprawlVehicleVisualForward.h"
#include "Vehicles/SprawlVehicleDriveLogic.h"
#include "Vehicles/SprawlVehicleOccupantPlacement.h"
#include "World/SprawlLightingContrast.h"
#include "World/SprawlRoadMarkingLayout.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using Grid = USprawlCityGridSubsystem;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlTrafficManeuverResolutionTest,
	"ConnectedSprawl.Traffic.Route.ManeuverResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlTrafficManeuverResolutionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	constexpr int32 SourceRoad = 2;
	constexpr int32 CrossingRoad = 4;

	for (int32 HeadingIndex = 0; HeadingIndex < 4; ++HeadingIndex)
	{
		const ESprawlHeading Heading = static_cast<ESprawlHeading>(HeadingIndex);
		for (const ESprawlTrafficManeuver Maneuver : {
			ESprawlTrafficManeuver::Straight,
			ESprawlTrafficManeuver::Right,
			ESprawlTrafficManeuver::Left,
			ESprawlTrafficManeuver::UTurn })
		{
			ESprawlHeading OutHeading = ESprawlHeading::East;
			int32 OutRoad = INDEX_NONE;
			FSprawlTrafficRoute::ResolveManeuver(
				Heading, SourceRoad, CrossingRoad, Maneuver,
				OutHeading, OutRoad);

			int32 ExpectedHeading = HeadingIndex;
			int32 ExpectedRoad = SourceRoad;
			switch (Maneuver)
			{
			case ESprawlTrafficManeuver::Right:
				ExpectedHeading = (HeadingIndex + 1) % 4;
				ExpectedRoad = CrossingRoad;
				break;
			case ESprawlTrafficManeuver::Left:
				ExpectedHeading = (HeadingIndex + 3) % 4;
				ExpectedRoad = CrossingRoad;
				break;
			case ESprawlTrafficManeuver::UTurn:
				ExpectedHeading = (HeadingIndex + 2) % 4;
				break;
			default:
				break;
			}

			const FString Label = FString::Printf(
				TEXT("heading=%d maneuver=%d"),
				HeadingIndex, static_cast<int32>(Maneuver));
			TestEqual(*FString::Printf(TEXT("%s heading"), *Label),
				static_cast<int32>(OutHeading), ExpectedHeading);
			TestEqual(*FString::Printf(TEXT("%s road"), *Label),
				OutRoad, ExpectedRoad);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlTrafficDeadEndTest,
	"ConnectedSprawl.Traffic.Route.ReservedUTurnAtLakeDeadEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlTrafficDeadEndTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	constexpr int32 RoadIndex = 5;
	constexpr int32 LakeCrossing = 1;
	const FVector SouthboundLocation(
		Grid::LaneCenter(RoadIndex, ESprawlHeading::South), -3332.f, 180.f);

	FSprawlTrafficApproach Approach;
	TestTrue(TEXT("Southbound car finds the lake-edge approach"),
		FSprawlTrafficRoute::TryGetNextApproach(
			SouthboundLocation, ESprawlHeading::South, RoadIndex, Approach));
	TestEqual(TEXT("The next crossing is the lake-edge road"),
		Approach.CrossingRoadIndex, LakeCrossing);
	TestFalse(TEXT("Straight would enter the lake"),
		FSprawlTrafficRoute::IsManeuverLegal(
			ESprawlHeading::South, RoadIndex, LakeCrossing,
			ESprawlTrafficManeuver::Straight));
	TestFalse(TEXT("Right would leave the authored road grid"),
		FSprawlTrafficRoute::IsManeuverLegal(
			ESprawlHeading::South, RoadIndex, LakeCrossing,
			ESprawlTrafficManeuver::Right));
	TestFalse(TEXT("Left would enter the lake"),
		FSprawlTrafficRoute::IsManeuverLegal(
			ESprawlHeading::South, RoadIndex, LakeCrossing,
			ESprawlTrafficManeuver::Left));
	TestTrue(TEXT("A U-turn remains on a dry directed lane"),
		FSprawlTrafficRoute::IsManeuverLegal(
			ESprawlHeading::South, RoadIndex, LakeCrossing,
			ESprawlTrafficManeuver::UTurn));
	TestEqual(TEXT("The dead end chooses a reserved U-turn"),
		static_cast<int32>(FSprawlTrafficRoute::ChooseManeuver(
			ESprawlHeading::South, RoadIndex, LakeCrossing, 0.1f)),
		static_cast<int32>(ESprawlTrafficManeuver::UTurn));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlTrafficSpawnRouteTest,
	"ConnectedSprawl.Traffic.Route.SpawnRequiresForwardExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlTrafficSpawnRouteTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	constexpr int32 RoadIndex = 5;
	const FVector SouthboundLocation(
		Grid::LaneCenter(RoadIndex, ESprawlHeading::South), -3332.f, 180.f);
	const FVector NorthboundLocation(
		Grid::LaneCenter(RoadIndex, ESprawlHeading::North), -3332.f, 180.f);

	TestFalse(TEXT("A spawn may not face a route whose only exit is a U-turn"),
		FSprawlTrafficRoute::IsSpawnRouteViable(
			SouthboundLocation, ESprawlHeading::South, RoadIndex));
	TestTrue(TEXT("The opposite directed lane has a dry forward route"),
		FSprawlTrafficRoute::IsSpawnRouteViable(
			NorthboundLocation, ESprawlHeading::North, RoadIndex));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlTrafficLaneDisciplineTest,
	"ConnectedSprawl.Traffic.Lane.GuidanceAndWrongWayDetection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlTrafficLaneDisciplineTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	constexpr int32 RoadIndex = 2;
	constexpr float Speed = 300.f;
	const float MidBlock = Grid::BlockCenter(3);

	for (int32 HeadingIndex = 0; HeadingIndex < 4; ++HeadingIndex)
	{
		const ESprawlHeading Heading = static_cast<ESprawlHeading>(HeadingIndex);
		const bool bNorthSouth = Grid::IsNorthSouth(Heading);
		const float LaneCenter = Grid::LaneCenter(RoadIndex, Heading);
		const FVector Location = bNorthSouth
			? FVector(LaneCenter, MidBlock, 180.f)
			: FVector(MidBlock, LaneCenter, 180.f);
		const FVector Direction = Grid::HeadingVector(Heading);
		const FString Label = FString::Printf(TEXT("heading=%d"), HeadingIndex);

		const FSprawlTrafficGuidance Guidance =
			FSprawlTrafficLaneDiscipline::ComputeGuidance(
				Location, Grid::HeadingYaw(Heading), Speed, Heading, RoadIndex);
		TestNearlyEqual(*FString::Printf(TEXT("%s exact lane error"), *Label),
			Guidance.LaneError, 0.f, 0.01f);
		TestNearlyEqual(*FString::Printf(TEXT("%s exact steering"), *Label),
			Guidance.SteeringInput, 0.f, 0.01f);
		TestNearlyEqual(*FString::Printf(TEXT("%s full speed"), *Label),
			Guidance.SpeedScale, 1.f, 0.01f);

		const FSprawlTrafficLaneSample ForwardSample =
			FSprawlTrafficLaneDiscipline::Measure(
				Location, Direction, Direction * Speed, Heading, RoadIndex);
		TestFalse(*FString::Printf(TEXT("%s forward travel is legal"), *Label),
			ForwardSample.bWrongWay);
		const FSprawlTrafficLaneSample ReverseSample =
			FSprawlTrafficLaneDiscipline::Measure(
				Location, Direction, Direction * -Speed, Heading, RoadIndex);
		TestTrue(*FString::Printf(TEXT("%s reverse velocity is wrong-way"), *Label),
			ReverseSample.bWrongWay);

		FVector MigratedLocation = Location;
		if (bNorthSouth)
		{
			MigratedLocation.X += 125.f;
		}
		else
		{
			MigratedLocation.Y += 125.f;
		}
		FVector CanonicalLocation = MigratedLocation;
		const bool bCanonicalized =
			FSprawlTrafficLaneDiscipline::BuildCanonicalLocation(
				MigratedLocation, Heading, RoadIndex, CanonicalLocation);
		TestTrue(*FString::Printf(TEXT("%s small authored offset is repairable"), *Label),
			bCanonicalized);
		if (bCanonicalized)
		{
			TestNearlyEqual(*FString::Printf(
				TEXT("%s canonical lane coordinate"), *Label),
				static_cast<float>(bNorthSouth
					? CanonicalLocation.X : CanonicalLocation.Y),
				LaneCenter, 0.01f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlVehicleVisualForwardTest,
	"ConnectedSprawl.Vehicles.VisualForward.NamedFrontAxle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlVehicleVisualForwardTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	struct FOrientationCase
	{
		FVector FrontLeft;
		FVector FrontRight;
		FVector RearLeft;
		FVector RearRight;
		float ExpectedYaw;
	};
	const TArray<FOrientationCase> Cases = {
		{ FVector(100.f, -40.f, 0.f), FVector(100.f, 40.f, 0.f),
			FVector(-100.f, -40.f, 0.f), FVector(-100.f, 40.f, 0.f), 0.f },
		{ FVector(-100.f, 40.f, 0.f), FVector(-100.f, -40.f, 0.f),
			FVector(100.f, 40.f, 0.f), FVector(100.f, -40.f, 0.f), 180.f },
		{ FVector(-40.f, 100.f, 0.f), FVector(40.f, 100.f, 0.f),
			FVector(-40.f, -100.f, 0.f), FVector(40.f, -100.f, 0.f), -90.f },
		{ FVector(40.f, -100.f, 0.f), FVector(-40.f, -100.f, 0.f),
			FVector(40.f, 100.f, 0.f), FVector(-40.f, 100.f, 0.f), 90.f }
	};
	for (int32 Index = 0; Index < Cases.Num(); ++Index)
	{
		const FOrientationCase& TestCase = Cases[Index];
		const FSprawlVehicleVisualForwardResult Result =
			FSprawlVehicleVisualForward::ResolveRelativeYaw(
				TestCase.FrontLeft, TestCase.FrontRight,
				TestCase.RearLeft, TestCase.RearRight);
		TestTrue(*FString::Printf(TEXT("orientation %d is valid"), Index), Result.bValid);
		TestNearlyEqual(*FString::Printf(TEXT("orientation %d forward yaw"), Index),
			FMath::FindDeltaAngleDegrees(
				TestCase.ExpectedYaw, Result.RelativeYawDegrees), 0.f, 0.01f);
	}

	const FRotator AuthoredRotation(7.f, 90.f, -3.f);
	const FRotator AxleAligned = FSprawlVehicleVisualForward::ResolveAlignedRotation(
		AuthoredRotation,
		FVector(-40.f, 100.f, 0.f), FVector(40.f, 100.f, 0.f),
		FVector(-40.f, -100.f, 0.f), FVector(40.f, -100.f, 0.f));
	TestNearlyEqual(TEXT("Named axle alignment corrects a stale positive yaw"),
		FMath::FindDeltaAngleDegrees(-90.f, AxleAligned.Yaw), 0.0, 0.01);
	TestNearlyEqual(TEXT("Named axle alignment retains authored pitch"),
		AxleAligned.Pitch, AuthoredRotation.Pitch, 0.01);
	TestNearlyEqual(TEXT("Named axle alignment retains authored roll"),
		AxleAligned.Roll, AuthoredRotation.Roll, 0.01);

	const FRotator BlenderAligned =
		FSprawlVehicleVisualForward::ResolveBlenderYForwardRotation(AuthoredRotation);
	TestNearlyEqual(TEXT("Blender Y-forward default points at physics forward"),
		FMath::FindDeltaAngleDegrees(-90.f, BlenderAligned.Yaw), 0.0, 0.01);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlVehicleOccupantAndDriveLogicTest,
	"ConnectedSprawl.Vehicles.OccupantContainmentAndDriveLogic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlVehicleOccupantAndDriveLogicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FBox SedanBounds(FVector(-240.f, -100.f, -62.f),
		FVector(240.f, 100.f, 88.f));
	const FSprawlVehicleOccupantSeat Seat =
		FSprawlVehicleOccupantPlacement::ComputeFromBodyBounds(SedanBounds);
	TestTrue(TEXT("Sedan produces a valid contained seat"), Seat.bValid);
	TestTrue(TEXT("Driver pelvis remains inside the cabin envelope"),
		FSprawlVehicleOccupantPlacement::IsContained(Seat));

	constexpr float Engine = 3200000.f;
	const FSprawlVehicleDriveCommand Forward =
		FSprawlVehicleDriveLogic::ResolvePlayerCommand(
			1.f, 0.5f, 300.f, Engine, 1.8f, 78.f);
	TestTrue(TEXT("Forward throttle drives the nose forward"),
		Forward.LongitudinalForce > 0.f && !Forward.bReversing);
	TestTrue(TEXT("Forward steering uses positive yaw"), Forward.YawRateDegrees > 0.f);

	const FSprawlVehicleDriveCommand Brake =
		FSprawlVehicleDriveLogic::ResolvePlayerCommand(
			-1.f, 0.f, 300.f, Engine, 1.8f, 78.f);
	TestTrue(TEXT("Reverse input first brakes forward motion"),
		Brake.bBraking && !Brake.bReversing);
	TestNearlyEqual(TEXT("Brake multiplier is applied"),
		Brake.LongitudinalForce, -Engine * 1.8f, 1.f);

	const FSprawlVehicleDriveCommand Reverse =
		FSprawlVehicleDriveLogic::ResolvePlayerCommand(
			-1.f, 0.5f, -120.f, Engine, 1.8f, 78.f);
	TestTrue(TEXT("A stopped/backward car deliberately reverses"), Reverse.bReversing);
	TestTrue(TEXT("Steering flips while reversing"), Reverse.YawRateDegrees < 0.f);

	const FVector Kinematic = FSprawlVehicleDriveLogic::BuildKinematicVelocity(
		FVector::ForwardVector, 850.f, FVector(0.f, 0.f, -98.f));
	TestNearlyEqual(TEXT("AI drive reaches target forward speed"), Kinematic.X, 850.0, 0.01);
	TestNearlyEqual(TEXT("AI drive preserves gravity velocity"), Kinematic.Z, -98.0, 0.01);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlLightingAndRoadMarkingLayoutTest,
	"ConnectedSprawl.World.LightingContrastAndRoadMarkingFit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlLightingAndRoadMarkingLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FSprawlLightingContrastProfile Day =
		FSprawlLightingContrast::BuildProfile(45.f);
	TestTrue(TEXT("Day profile reduces flat ambient fill"),
		Day.SkyLightMultiplier < 1.f && Day.FogDensityMultiplier < 1.f);
	TestTrue(TEXT("Day profile retains readable direct light"),
		Day.DirectLightMultiplier >= 0.95f);

	const FSprawlRoadMarkingFit Fit = FSprawlRoadMarkingLayout::GetFit();
	TestTrue(TEXT("Paint is a thin surface treatment"), Fit.MeshThickness < 0.02f);
	TestTrue(TEXT("Paint sits above the road without a raised slab"),
		Fit.SurfaceZ > 0.f && Fit.SurfaceZ < 1.f);

	TArray<FSprawlRoadMarkingSegment> Intervals;
	FSprawlRoadMarkingLayout::BuildRoadIntervals(Intervals);
	TestEqual(TEXT("Every road crossing creates two approach-safe bounds"),
		Intervals.Num(), Grid::NumRoads + 1);
	TArray<float> DashCenters;
	FSprawlRoadMarkingLayout::AppendCenteredDashCenters(
		Intervals, 190.f, 520.f, DashCenters);
	for (const float Center : DashCenters)
	{
		for (int32 RoadIndex = 0; RoadIndex < Grid::NumRoads; ++RoadIndex)
		{
			TestTrue(TEXT("Dash clears stop-line approach space"),
				FMath::Abs(Center - Grid::RoadCenter(RoadIndex))
					>= Grid::StopLineDistance + 80.f + 95.f - 0.01f);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
