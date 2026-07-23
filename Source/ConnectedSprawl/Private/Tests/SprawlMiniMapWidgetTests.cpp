// The Connected Sprawl - corner minimap projection tests.

#include "UI/SprawlMiniMapWidget.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlMiniMapWidgetTest,
	"ConnectedSprawl.UI.MiniMapWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlMiniMapWidgetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	constexpr float Span = 9000.f;
	constexpr float Size = 216.f;
	constexpr float Centre = Size * 0.5f;
	const FVector Viewer(1500.f, -2400.f, 90.f);

	const FVector2D Self =
		USprawlMiniMapWidget::ProjectToMiniMap(Viewer, Viewer, Span, Size);
	TestEqual(TEXT("The viewer sits at the centre of the face"), Self,
		FVector2D(Centre, Centre));

	// North is world +Y and must rise toward the top edge (smaller pixel Y).
	const FVector2D North = USprawlMiniMapWidget::ProjectToMiniMap(
		Viewer + FVector(0.f, 1000.f, 0.f), Viewer, Span, Size);
	TestTrue(TEXT("North of the viewer draws above centre"), North.Y < Centre);
	TestEqual(TEXT("North of the viewer stays on the centre column"),
		static_cast<float>(North.X), Centre);

	const FVector2D East = USprawlMiniMapWidget::ProjectToMiniMap(
		Viewer + FVector(1000.f, 0.f, 0.f), Viewer, Span, Size);
	TestTrue(TEXT("East of the viewer draws right of centre"), East.X > Centre);
	TestEqual(TEXT("East of the viewer stays on the centre row"),
		static_cast<float>(East.Y), Centre);

	// Half a view span east is exactly the right edge: the scale is 1:1 with span.
	const FVector2D Edge = USprawlMiniMapWidget::ProjectToMiniMap(
		Viewer + FVector(Span * 0.5f, 0.f, 0.f), Viewer, Span, Size);
	TestEqual(TEXT("Half a span east lands on the right edge"),
		static_cast<float>(Edge.X), Size);

	const FVector2D Altitude = USprawlMiniMapWidget::ProjectToMiniMap(
		Viewer + FVector(0.f, 0.f, 5000.f), Viewer, Span, Size);
	TestEqual(TEXT("Height above the viewer does not move the marker"),
		Altitude, FVector2D(Centre, Centre));

	const FVector2D DegenerateSpan =
		USprawlMiniMapWidget::ProjectToMiniMap(Viewer, Viewer, 0.f, Size);
	TestTrue(TEXT("A zero view span still produces finite pixels"),
		FMath::IsFinite(DegenerateSpan.X) && FMath::IsFinite(DegenerateSpan.Y));

	// An up-pointing glyph already reads as north, so facing north needs no spin.
	TestEqual(TEXT("Facing north leaves the marker upright"),
		USprawlMiniMapWidget::ResolveMarkerAngle(90.f), 0.f);
	TestEqual(TEXT("Facing east turns the marker a quarter clockwise"),
		USprawlMiniMapWidget::ResolveMarkerAngle(0.f), 90.f);
	TestEqual(TEXT("Facing south inverts the marker"),
		USprawlMiniMapWidget::ResolveMarkerAngle(-90.f), 180.f);
	const float WrappedAngle = USprawlMiniMapWidget::ResolveMarkerAngle(450.f);
	TestTrue(TEXT("A wrapped yaw stays inside one turn"),
		WrappedAngle >= 0.f && WrappedAngle < 360.f);

	TestTrue(TEXT("The centre pixel is on the face"),
		USprawlMiniMapWidget::IsPixelOnFace(
			FVector2D(Centre, Centre), Size, 10.f));
	TestFalse(TEXT("A pixel well beyond the margin is off the face"),
		USprawlMiniMapWidget::IsPixelOnFace(
			FVector2D(Size + 40.f, Centre), Size, 10.f));
	TestFalse(TEXT("A non-finite pixel is never on the face"),
		USprawlMiniMapWidget::IsPixelOnFace(
			FVector2D(NAN, Centre), Size, 10.f));

	return true;
}

#endif
