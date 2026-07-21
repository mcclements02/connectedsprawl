"""Author clean low-poly mountain ridges in headless Blender.

The city horizon currently leans on a marketplace vista mesh that reads as
pixelated tan lumps with hard white blobs. This bakes two deliberate,
flat-shaded silhouette ridges instead:

  * SM_MountainRidgeA — broad three-peak range (400 x 80 m footprint)
  * SM_MountainRidgeB — sharper two-peak ridge  (280 x 60 m footprint)

Shape = gaussian peak envelope x cosine cross-falloff + Perlin fBm rockiness,
then a collapse-decimate for the low-poly facet look. Colour lives entirely in
CORNER vertex colours (rock/scree by noise and height, snow by altitude +
face slope), so the runtime material is a two-node vertex-colour shader with
zero textures — the cheapest possible mobile background.

Run (deterministic):
  /opt/homebrew/bin/blender --background --python Tools/build_mountain_kit.py
"""
import math
import os
import random

import bpy
from mathutils import Vector, noise

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, "Content", "Import", "CityKit")

lines = []


def out(text=""):
    lines.append(text)
    print("[MountainKit] " + text)


def fbm(x, y, seed_offset, octaves=4):
    total, amp, freq = 0.0, 1.0, 1.0
    for _ in range(octaves):
        total += amp * noise.noise(
            Vector((x * freq, y * freq, seed_offset)))
        amp *= 0.5
        freq *= 2.1
    return total


def build_ridge(name, length_m, depth_m, peaks, noise_amp, seed, decimate):
    """peaks: list of (x_fraction, height_m, width_m)."""
    nx, ny = 120, 24
    verts = []
    for iy in range(ny + 1):
        for ix in range(nx + 1):
            fx = ix / nx
            fy = iy / ny
            x = (fx - 0.5) * length_m
            y = (fy - 0.5) * depth_m
            # peak envelope along the ridge
            env = 0.0
            for px, ph, pw in peaks:
                d = (fx - px) * length_m
                env = max(env, ph * math.exp(-(d * d) / (2.0 * pw * pw)))
            # cosine falloff across the ridge so edges sit on the ground
            cross = 0.5 * (1.0 + math.cos(math.pi * (fy - 0.5) * 2.0))
            z = env * cross
            if z > 0.5:
                z += noise_amp * cross * fbm(fx * 6.0, fy * 3.0, seed)
                z = max(z, 0.0)
            verts.append((x, y, z))
    faces = []
    for iy in range(ny):
        for ix in range(nx):
            a = iy * (nx + 1) + ix
            b = a + 1
            c = a + (nx + 1) + 1
            d = a + (nx + 1)
            faces.append((a, b, c, d))

    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    # collapse-decimate for the clean faceted silhouette
    mod = obj.modifiers.new("dec", "DECIMATE")
    mod.ratio = decimate
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier="dec")
    mesh = obj.data
    bpy.ops.object.shade_flat()

    # ---- vertex colours: rock / scree / snow --------------------------------
    max_z = max(v.co.z for v in mesh.vertices) or 1.0
    snow_line = max_z * 0.52
    rand = random.Random(seed)
    attr = mesh.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
    rock_a = (0.235, 0.205, 0.180)   # warm grey-brown
    rock_b = (0.160, 0.150, 0.140)   # cooler shadow rock
    scree = (0.290, 0.262, 0.215)    # sandy base apron
    snow = (0.780, 0.800, 0.830)

    loop_i = 0
    for poly in mesh.polygons:
        up = poly.normal.z
        for li in poly.loop_indices:
            v = mesh.vertices[mesh.loops[li].vertex_index]
            t = fbm(v.co.x * 0.02, v.co.y * 0.02, seed + 7.0) * 0.5 + 0.5
            col = [rock_a[i] + (rock_b[i] - rock_a[i]) * t for i in range(3)]
            if v.co.z < max_z * 0.10:
                f = 1.0 - v.co.z / (max_z * 0.10)
                col = [col[i] + (scree[i] - col[i]) * f for i in range(3)]
            if v.co.z > snow_line and up > 0.45:
                f = min(1.0, (v.co.z - snow_line) / (max_z - snow_line) + 0.35)
                f *= min(1.0, (up - 0.45) / 0.35)
                col = [col[i] + (snow[i] - col[i]) * f for i in range(3)]
            g = 1.0 + (rand.random() - 0.5) * 0.06
            attr.data[loop_i].color = (col[0] * g, col[1] * g, col[2] * g, 1.0)
            loop_i += 1

    tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
    out("%s: %.0fx%.0fm peak=%.0fm tris=%d" % (
        name, length_m, depth_m, max_z, tris))
    return obj


def export_fbx(obj, name):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUT_DIR, name + ".fbx")
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, object_types={"MESH"},
        apply_unit_scale=True, global_scale=1.0, bake_space_transform=True,
        use_mesh_modifiers=True, colors_type="SRGB", add_leaf_bones=False,
        path_mode="AUTO")
    out("exported %s.fbx (%.1f KB)" % (name, os.path.getsize(path) / 1024.0))


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    bpy.ops.wm.read_homefile(use_empty=True)
    ridge_a = build_ridge(
        "SM_MountainRidgeA", 400.0, 80.0,
        peaks=[(0.20, 62.0, 42.0), (0.50, 88.0, 55.0), (0.80, 70.0, 46.0)],
        noise_amp=7.5, seed=11.0, decimate=0.34)
    export_fbx(ridge_a, "SM_MountainRidgeA")
    ridge_b = build_ridge(
        "SM_MountainRidgeB", 280.0, 60.0,
        peaks=[(0.34, 52.0, 34.0), (0.70, 74.0, 40.0)],
        noise_amp=10.0, seed=23.0, decimate=0.30)
    export_fbx(ridge_b, "SM_MountainRidgeB")
    with open(os.path.join(OUT_DIR, "MountainKitReport.txt"), "w") as fh:
        fh.write("\n".join(lines) + "\n")
    out("DONE")


main()
