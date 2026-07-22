from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).with_name("character_developer.py")
SPEC = importlib.util.spec_from_file_location("character_developer", MODULE_PATH)
assert SPEC and SPEC.loader
character_developer = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(character_developer)


class CharacterDeveloperTests(unittest.TestCase):
    def test_offline_profile_is_deterministic_and_valid(self) -> None:
        args = {
            "seed": 42,
            "district": "RailYards",
            "role": "WarehouseWorker",
            "city_hour": 6.5,
            "request": "A veteran loading-dock coordinator on the first shift.",
        }
        first = character_developer.build_offline_profile(**args)
        repeat = character_developer.build_offline_profile(**args)
        self.assertEqual(first, repeat)
        character_developer.validate_profile(first)
        self.assertEqual(first["character_id"], "City_0000002A")
        self.assertIn(
            first["preferred_avatar_variant"],
            {"Baldman", "BizDude", "Chill", "Erika", "Kyle", "Lydia", "Samuela", "Bruno"},
        )
        self.assertIn("Optimized/Low", first["metahuman_creator_brief"])
        self.assertIn("Front, left profile, and back", first["reference_image_prompt"])

    def test_json_extraction_accepts_a_fenced_response(self) -> None:
        parsed = character_developer._extract_json('```json\n{"ok": true}\n```')
        self.assertEqual(parsed, {"ok": True})

    def test_validation_rejects_out_of_range_behavior(self) -> None:
        profile = character_developer.build_offline_profile(
            seed=7,
            district="Junction",
            role="Courier",
            city_hour=12.0,
            request="Lunch courier.",
        )
        profile["cross_chance"] = 2.0
        with self.assertRaisesRegex(ValueError, "cross_chance"):
            character_developer.validate_profile(profile)


if __name__ == "__main__":
    unittest.main()
