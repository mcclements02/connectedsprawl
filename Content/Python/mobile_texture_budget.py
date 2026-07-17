"""Clamp oversized Downtown West textures for the iPhone budget (nullrhi-safe).

Ledger item: the fountain ships 4K masters (~30MB each) and the background
mountain an 8K pair (102MB normal). Clamp via MaxTextureSize so source data
stays intact but cooked/streamed mips respect the mobile budget:
  - prop_fountain + fountain VFX masks -> 1024
  - background_mountain color -> 2048, normal -> 1024 (distant vista)

Edits live in the untracked Fab pack copy — re-downloading the pack reverts
them; re-run this script after any pack refresh.
"""
import unreal

eal = unreal.EditorAssetLibrary

GROUPS = [
    ("/Game/Downtown_West/Assets/props/prop_fountain/Textures", 1024, 1024),
    ("/Game/Downtown_West/VFX/Textures", 1024, 1024),
    ("/Game/Downtown_West/Assets/background_mountain/Textures", 2048, 1024),
]

changed = 0
for root, color_max, normal_max in GROUPS:
    if not eal.does_directory_exist(root):
        unreal.log_warning("[TexBudget] missing dir {}".format(root))
        continue
    for path in eal.list_assets(root, recursive=True):
        asset = eal.load_asset(path)
        if not isinstance(asset, unreal.Texture2D):
            continue
        is_normal = asset.get_editor_property("compression_settings") == \
            unreal.TextureCompressionSettings.TC_NORMALMAP
        target = normal_max if is_normal else color_max
        current = asset.get_editor_property("max_texture_size")
        if current != 0 and current <= target:
            continue
        asset.set_editor_property("max_texture_size", target)
        eal.save_loaded_asset(asset)
        changed += 1
        unreal.log("[TexBudget] {} -> max {}".format(path, target))

unreal.log("[TexBudget] done, {} textures clamped".format(changed))
