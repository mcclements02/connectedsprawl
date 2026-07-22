// The Connected Sprawl - city-map landmark and projection tests.

#include "World/SprawlCityMapSubsystem.h"

#include "Misc/AutomationTest.h"
#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlEnterableInteriors.h"

#if WITH_DEV_AUTOMATION_TESTS

using Grid = USprawlCityGridSubsystem;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlCityMapLayoutTest,
	"ConnectedSprawl.UI.CityMapLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlCityMapLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const TArray<FSprawlMapLandmark> Landmarks =
		USprawlCityMapSubsystem::BuildDefaultLandmarks();
	TestEqual(TEXT("Map contains three destinations plus parking and waterfront"),
		Landmarks.Num(), 5);

	TSet<FName> Ids;
	int32 EnterableCount = 0;
	for (const FSprawlMapLandmark& Landmark : Landmarks)
	{
		const FString Label = Landmark.Label.ToString();
		TestTrue(*FString::Printf(TEXT("%s marker is valid"), *Label),
			Landmark.IsValid());
		TestFalse(*FString::Printf(TEXT("%s marker id is unique"), *Label),
			Ids.Contains(Landmark.Id));
		TestTrue(*FString::Printf(TEXT("%s marker remains on-map"), *Label),
			Grid::IsInsideCityBounds(
				Landmark.WorldLocation.X, Landmark.WorldLocation.Y));
		Ids.Add(Landmark.Id);
		EnterableCount += Landmark.bEnterable ? 1 : 0;
	}
	TestEqual(TEXT("Exactly the store, office, and condo are enterable"),
		EnterableCount, 3);

	const FVector2D Center = USprawlCityMapSubsystem::WorldToMapNormalized(
		FVector::ZeroVector);
	TestNearlyEqual(TEXT("World center maps to horizontal center"),
		Center.X, 0.5, 0.001);
	TestNearlyEqual(TEXT("World center maps to vertical center"),
		Center.Y, 0.5, 0.001);
	const FVector2D MinCorner = USprawlCityMapSubsystem::WorldToMapNormalized(
		FVector(-Grid::CityBoundaryHalfExtent, -Grid::CityBoundaryHalfExtent, 0.f));
	const FVector2D MaxCorner = USprawlCityMapSubsystem::WorldToMapNormalized(
		FVector(Grid::CityBoundaryHalfExtent, Grid::CityBoundaryHalfExtent, 0.f));
	TestTrue(TEXT("South-west city corner maps to zero"),
		MinCorner.Equals(FVector2D::ZeroVector, 0.001f));
	TestTrue(TEXT("North-east city corner maps to one"),
		MaxCorner.Equals(FVector2D(1.f, 1.f), 0.001f));
	const FVector2D Clamped = USprawlCityMapSubsystem::WorldToMapNormalized(
		FVector(999999.f, -999999.f, 0.f));
	TestTrue(TEXT("Off-map positions clamp into the visible map"),
		Clamped.Equals(FVector2D(1.f, 0.f), 0.001f));

	const FSprawlEnterableInteriorsLayout Interiors =
		FSprawlEnterableInteriorsLayout::Build();
	for (const FSprawlEnterableBuildingSpec& Building : Interiors.Buildings)
	{
		const FSprawlMapLandmark* Marker = Landmarks.FindByPredicate(
			[&](const FSprawlMapLandmark& Candidate)
			{
				return Candidate.Id == Building.Id;
			});
		TestNotNull(*FString::Printf(TEXT("%s appears on the map"),
			*Building.DisplayName.ToString()), Marker);
		if (Marker)
		{
			TestTrue(TEXT("Building marker equals its exterior doorway"),
				Marker->WorldLocation.Equals(
					Building.ExteriorReturnLocation, 0.01f));
		}
	}
	return true;
}

#endif
