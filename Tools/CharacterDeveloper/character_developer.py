#!/usr/bin/env python3
"""Develop city-character profiles for Unreal and MetaHuman Creator.

The default path calls Hugging Face's OpenAI-compatible inference router.  It
never runs in the game and never stores credentials.  ``--offline`` provides a
deterministic profile for tests, CI, and workstations without a Hub token.
"""

from __future__ import annotations

import argparse
import json
import os
import random
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
DEFAULT_PROFILE_MODEL = "Qwen/Qwen3.5-4B"
DEFAULT_REFERENCE_MODEL = "black-forest-labs/FLUX.1-schnell"
ROUTER_URL = "https://router.huggingface.co/v1/chat/completions"

DISTRICTS = ("Junction", "IronForest", "RailYards", "Arteries")
ROLES = (
    "Resident",
    "Commuter",
    "TechWorker",
    "ServiceWorker",
    "WarehouseWorker",
    "Courier",
    "Founder",
    "Student",
    "NightWorker",
)
WARDROBES = ("Streetwear", "SmartCasual", "Corporate", "Workwear", "Athletic")
ACTIVITIES = (
    "Commuting",
    "Working",
    "RunningErrand",
    "Socializing",
    "Exercising",
    "HeadingHome",
)

FIRST_NAMES = (
    "Amina",
    "Andre",
    "Camila",
    "Darius",
    "Elena",
    "Imani",
    "Jae",
    "Jordan",
    "Luis",
    "Maya",
    "Nia",
    "Omar",
    "Priya",
    "Rafael",
    "Sam",
    "Tasha",
)


def _wardrobe_for(role: str) -> str:
    if role == "TechWorker":
        return "SmartCasual"
    if role == "Commuter":
        return "Corporate"
    if role in {"WarehouseWorker", "NightWorker"}:
        return "Workwear"
    if role == "Courier":
        return "Athletic"
    return "Streetwear"


def _schedule_for(role: str) -> tuple[float, float]:
    return {
        "NightWorker": (20.0, 6.0),
        "WarehouseWorker": (6.0, 18.0),
        "Courier": (8.0, 22.0),
        "Student": (8.0, 19.0),
        "ServiceWorker": (7.0, 23.0),
    }.get(role, (7.0, 20.0))


def _avatar_for(wardrobe: str, rng: random.Random) -> str:
    pools = {
        "Streetwear": ("Chill", "Kyle", "Samuela", "Bruno", "Erika"),
        "SmartCasual": ("Erika", "Lydia", "BizDude", "Bruno"),
        "Corporate": ("BizDude", "Erika", "Lydia"),
        "Workwear": ("Baldman", "Bruno", "Samuela", "Kyle"),
        "Athletic": ("Chill", "Kyle", "Samuela", "Bruno"),
    }
    return rng.choice(pools[wardrobe])


def build_offline_profile(
    *, seed: int, district: str, role: str, city_hour: float, request: str
) -> dict[str, Any]:
    """Create a schema-valid profile without a network or third-party package."""

    rng = random.Random(seed)
    wardrobe = _wardrobe_for(role)
    start, end = _schedule_for(role)
    rush = 7.0 <= city_hour < 10.0 or 16.0 <= city_hour < 19.0
    if rush:
        activity = "Commuting"
    elif city_hour >= 21.0 or city_hour < 6.0:
        activity = "Working" if role in {"NightWorker", "WarehouseWorker", "Courier"} else "HeadingHome"
    elif 10.0 <= city_hour < 17.0 and role not in {"Resident", "Student"}:
        activity = "Working"
    else:
        activity = rng.choice(("RunningErrand", "Socializing", "Exercising"))

    display_name = f"{rng.choice(FIRST_NAMES)} {chr(ord('A') + rng.randrange(24))}."
    height_cm = round(rng.uniform(158.0, 190.0), 1)
    human_role = re.sub(r"(?<!^)(?=[A-Z])", " ", role).lower()
    human_district = {
        "Junction": "The Junction",
        "IronForest": "Iron Forest",
        "RailYards": "Rail Yards",
        "Arteries": "the 85-Express arteries",
    }[district]
    wardrobe_words = re.sub(r"(?<!^)(?=[A-Z])", "-", wardrobe).lower()
    brief = (
        f"Create {display_name}, a {human_role} based in {human_district}, "
        f"standing {height_cm:.0f} cm. Use practical {wardrobe_words} layers, "
        "natural skin detail, subtle facial asymmetry, practical grooming, and "
        "no visible brand logos. Assemble Optimized/Low with hair cards, a "
        "joints-only rig, and 2K source textures for the iPhone crowd budget."
    )
    reference_prompt = (
        f"Realistic full-body character turnaround sheet for {display_name}, "
        f"a {human_role} from {human_district}, wearing {wardrobe_words}. "
        "Front, left profile, and back views at matching scale, neutral A-pose, "
        "85mm lens look, even studio light, plain light-gray background, natural "
        "human proportions, clear garment seams and footwear, no logos or text."
    )
    return {
        "schema_version": SCHEMA_VERSION,
        "character_id": f"City_{seed & 0xFFFFFFFF:08X}",
        "display_name": display_name,
        "seed": seed,
        "home_district": district,
        "work_district": district,
        "role": role,
        "activity": activity,
        "destination": "a plausible destination in the current district",
        "active_start_hour": start,
        "active_end_hour": end,
        "wardrobe": wardrobe,
        "preferred_avatar_variant": _avatar_for(wardrobe, rng),
        "height_cm": height_cm,
        "walk_speed_scale": round(rng.uniform(0.88, 1.12), 3),
        "cross_chance": 0.30,
        "idle_talk_chance": 0.20,
        "backstory": (
            f"A grounded {human_district} resident whose visible routine supports "
            f"the city's daily economy. Design request: {request.strip()}"
        ),
        "voice_notes": "Natural, concise, locally grounded; avoid caricature.",
        "metahuman_creator_brief": brief,
        "reference_image_prompt": reference_prompt,
        "reference_model": DEFAULT_REFERENCE_MODEL,
        "source_model": "offline-deterministic",
    }


def build_generation_prompt(
    *, seed: int, district: str, role: str, city_hour: float, request: str
) -> str:
    return f"""Develop one grounded civilian for The Connected Sprawl.

Designer request: {request}
Seed: {seed}
Current district: {district}
Requested role: {role}
Current city hour: {city_hour:.2f}

Return one JSON object only. Use exactly these keys:
schema_version, character_id, display_name, seed, home_district, work_district,
role, activity, destination, active_start_hour, active_end_hour, wardrobe,
preferred_avatar_variant, height_cm, walk_speed_scale, cross_chance,
idle_talk_chance, backstory,
voice_notes, metahuman_creator_brief, reference_image_prompt, reference_model.

Enum constraints:
- home_district/work_district: {', '.join(DISTRICTS)}
- role: {', '.join(ROLES)}
- activity: {', '.join(ACTIVITIES)}
- wardrobe: {', '.join(WARDROBES)}
- preferred_avatar_variant: Baldman, BizDude, Chill, Erika, Kyle, Lydia, Samuela, Bruno

Keep the person specific but ordinary enough to repeat in a believable city.
Do not infer personality, morality, income, or criminality from protected traits.
Use a plausible adult height from 150 to 205 cm. Behavior chances must be 0..1.
The MetaHuman brief must require Optimized/Low, hair cards, joints-only rig, and
2K textures. The image prompt must request front/profile/back neutral views,
plain background, consistent scale, no logos, and no text. Set reference_model
to {DEFAULT_REFERENCE_MODEL!r} and schema_version to {SCHEMA_VERSION}.
"""


def _extract_json(text: str) -> dict[str, Any]:
    stripped = text.strip()
    if stripped.startswith("```"):
        stripped = re.sub(r"^```(?:json)?\s*", "", stripped, flags=re.IGNORECASE)
        stripped = re.sub(r"\s*```$", "", stripped)
    start = stripped.find("{")
    end = stripped.rfind("}")
    if start < 0 or end <= start:
        raise ValueError("Hugging Face response did not contain a JSON object")
    value = json.loads(stripped[start : end + 1])
    if not isinstance(value, dict):
        raise ValueError("Hugging Face response must be a JSON object")
    return value


def request_hugging_face_profile(*, model: str, token: str, prompt: str) -> dict[str, Any]:
    body = json.dumps(
        {
            "model": model,
            "messages": [
                {
                    "role": "system",
                    "content": (
                        "You are a character developer for an iPhone-first Unreal "
                        "Engine city. Return strict JSON and honor the supplied schema."
                    ),
                },
                {"role": "user", "content": prompt},
            ],
            "temperature": 0.65,
            "max_tokens": 1400,
        }
    ).encode("utf-8")
    request = urllib.request.Request(
        ROUTER_URL,
        data=body,
        method="POST",
        headers={
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json",
            "User-Agent": "ConnectedSprawl-CharacterDeveloper/1",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=90) as response:
            payload = json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")[:500]
        raise RuntimeError(f"Hugging Face returned HTTP {error.code}: {detail}") from error
    except urllib.error.URLError as error:
        raise RuntimeError(f"Could not reach Hugging Face: {error.reason}") from error

    try:
        content = payload["choices"][0]["message"]["content"]
    except (KeyError, IndexError, TypeError) as error:
        raise RuntimeError("Hugging Face returned an unexpected response shape") from error
    return _extract_json(content)


def validate_profile(profile: dict[str, Any]) -> None:
    required = {
        "schema_version",
        "character_id",
        "display_name",
        "seed",
        "home_district",
        "work_district",
        "role",
        "activity",
        "destination",
        "active_start_hour",
        "active_end_hour",
        "wardrobe",
        "preferred_avatar_variant",
        "height_cm",
        "walk_speed_scale",
        "cross_chance",
        "idle_talk_chance",
        "backstory",
        "voice_notes",
        "metahuman_creator_brief",
        "reference_image_prompt",
        "reference_model",
        "source_model",
    }
    missing = sorted(required - profile.keys())
    if missing:
        raise ValueError(f"Profile is missing required keys: {', '.join(missing)}")
    if profile["schema_version"] != SCHEMA_VERSION:
        raise ValueError(f"schema_version must be {SCHEMA_VERSION}")
    if profile["home_district"] not in DISTRICTS or profile["work_district"] not in DISTRICTS:
        raise ValueError("Invalid district")
    if profile["role"] not in ROLES or profile["activity"] not in ACTIVITIES:
        raise ValueError("Invalid role or activity")
    if profile["wardrobe"] not in WARDROBES:
        raise ValueError("Invalid wardrobe")
    if not 150.0 <= float(profile["height_cm"]) <= 205.0:
        raise ValueError("height_cm must be between 150 and 205")
    if not 0.6 <= float(profile["walk_speed_scale"]) <= 1.5:
        raise ValueError("walk_speed_scale must be between 0.6 and 1.5")
    for key in ("cross_chance", "idle_talk_chance"):
        if not 0.0 <= float(profile[key]) <= 1.0:
            raise ValueError(f"{key} must be between 0 and 1")
    brief = str(profile["metahuman_creator_brief"])
    if "Optimized/Low" not in brief or "hair cards" not in brief:
        raise ValueError("MetaHuman brief is missing the mobile assembly contract")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prompt", required=True, help="Character intent from the designer")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--district", choices=DISTRICTS, default="Junction")
    parser.add_argument("--role", choices=ROLES, default="Resident")
    parser.add_argument("--city-hour", type=float, default=8.07153)
    parser.add_argument("--model", default=DEFAULT_PROFILE_MODEL)
    parser.add_argument("--offline", action="store_true", help="Do not call Hugging Face")
    parser.add_argument("--output", type=Path, help="Write JSON to this path instead of stdout")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    if args.offline:
        profile = build_offline_profile(
            seed=args.seed,
            district=args.district,
            role=args.role,
            city_hour=args.city_hour % 24.0,
            request=args.prompt,
        )
    else:
        token = os.environ.get("HF_TOKEN", "").strip()
        if not token:
            raise RuntimeError(
                "HF_TOKEN is not set. Export a Hugging Face user token or pass --offline."
            )
        prompt = build_generation_prompt(
            seed=args.seed,
            district=args.district,
            role=args.role,
            city_hour=args.city_hour % 24.0,
            request=args.prompt,
        )
        profile = request_hugging_face_profile(model=args.model, token=token, prompt=prompt)
        profile["source_model"] = args.model

    validate_profile(profile)
    rendered = json.dumps(profile, indent=2, ensure_ascii=False) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        sys.stdout.write(rendered)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, ValueError) as error:
        print(f"character_developer: {error}", file=sys.stderr)
        raise SystemExit(2)
