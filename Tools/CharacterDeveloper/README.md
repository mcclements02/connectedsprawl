# Character Developer

This is an offline authoring tool for creating grounded civilian profiles,
MetaHuman Creator briefs, and full-body reference-sheet prompts. It does not
ship model weights, tokens, Python packages, or network code in the game.

The default profile model is
[`Qwen/Qwen3.5-4B`](https://huggingface.co/Qwen/Qwen3.5-4B), an Apache-2.0,
endpoint-compatible small multimodal model current when this pass was authored.
Reference prompts name
[`black-forest-labs/FLUX.1-schnell`](https://huggingface.co/black-forest-labs/FLUX.1-schnell),
also Apache-2.0. A model can be replaced with `--model` without changing the
Unreal runtime contract. Always review a replacement model card and license.

Generate with Hugging Face:

```sh
export HF_TOKEN="your Hugging Face user token"
python3 Tools/CharacterDeveloper/character_developer.py \
  --prompt "A late-shift transit dispatcher who lives in The Junction" \
  --district Junction --role NightWorker --seed 1042 \
  --output Design/Characters/transit_dispatcher.json
```

Never pass a token on the command line or store it in the repository. For a
network-free deterministic profile:

```sh
python3 Tools/CharacterDeveloper/character_developer.py \
  --offline --prompt "Morning warehouse supervisor" \
  --district RailYards --role WarehouseWorker --seed 42
```

The JSON is a design handoff. Create a `SprawlCharacterDefinition` Data Asset,
copy the approved profile fields, build the person in MetaHuman Creator, and
assign only an **Optimized/Low** assembled class. Crowd extras continue to use
the lightweight avatar named by the runtime profile; named mission characters
may opt into the soft MetaHuman class and locomotion references.

Run the tool tests with:

```sh
python3 -m unittest Tools/CharacterDeveloper/test_character_developer.py
```
