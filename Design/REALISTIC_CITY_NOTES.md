# Realistic City Environment - Research & Implementation Notes

**Project:** The Connected Sprawl
**Date:** 2026-05-31
**Goal:** Make pedestrians, vehicles, and world environment look realistic

---

## 📋 Current State Analysis

### Existing Systems
- **Pedestrians** (`SprawlPedestrian.cpp`): Basic wandering AI with simple collision avoidance
  - Uses UE4 Mannequin mesh
  - Random direction changes every 3-7 seconds
  - Simple stuck detection
  - RVO avoidance enabled

- **Vehicles** (`SprawlCar.cpp`): Physics-based car with basic geometry
  - Kitbashed cube/cylinder primitives for body
  - Static wheel meshes (no rotation animation)
  - Physics hull with locked X/Y rotation
  - Auto-drive AI for traffic

- **World**: Basic materials for buildings, roads, grass
  - Simple material instances
  - No foliage system
  - Basic procedural traffic spawning

---

## 🎯 Improvement Goals

### 1. Realistic Pedestrians
- [ ] Replace UE4 Mannequin with diverse, realistic characters
- [ ] Implement natural walking animations
- [ ] Add variety in clothing, body types, ages
- [ ] Improve AI behaviors (crossing streets, waiting at lights, interacting)
- [ ] Add ambient animations (phone usage, conversations, sitting)

### 2. Realistic Vehicles
- [ ] Replace primitive geometry with detailed car models
- [ ] Implement animated wheel rotation
- [ ] Add suspension animation
- [ ] Improve vehicle variety (sedans, SUVs, trucks)
- [ ] Add realistic headlights/taillights
- [ ] Implement brake lights and turn signals

### 3. Realistic World Environment
- [ ] Add detailed building facades with proper LODs
- [ ] Implement foliage system (grass, trees, bushes)
- [ ] Add urban props (trash cans, benches, signs, lights)
- [ ] Improve road materials with wear and markings
- [ ] Add ambient life (birds, wind effects)

---

## 🔍 Reference Projects & Resources

### Open Source GTA-Style Games

#### **San Andreas Unity**
- **Repo:** https://github.com/in0finite/SanAndreasUnity
- **Engine:** Unity
- **Features:**
  - Full vehicle physics with wheel animation
  - Pedestrian AI with navigation
  - City traffic system
  - Mission framework
- **Key Learnings:**
  - Modular traffic spawning around player
  - NavMesh-based pedestrian movement
  - Vehicle pooling for performance

#### **GTA Clone Prototype**
- **Repo:** https://github.com/akshayyrathore/Gta-clone-on-
- **Engine:** Unity/Unreal flexible
- **Features:**
  - Third-person controller
  - Vehicle enter/exit system
  - Basic AI and missions
- **Key Learnings:**
  - Simple but effective character switching
  - Basic traffic patterns

#### **Gorka Games UE5 GTA Tutorial**
- **Source:** YouTube tutorial series with free project files
- **Engine:** Unreal Engine 5
- **Features:**
  - Full city environment
  - Moving pedestrians and cars
  - Police AI and wanted system
  - Phone apps and parkour
- **Key Learnings:**
  - UE5-specific implementations
  - Modern BP patterns for open world

---

## 🚶 Pedestrian System Improvements

### Recommended Approach: Mass Entity Framework

**Mass AI** (UE5 native) is designed for large-scale crowd simulation:
- Data-oriented architecture
- Can handle 10,000+ agents
- Efficient LOD system
- Integrated with UE5 animation system

### Implementation Steps

1. **Character Assets**
   - **MetaHuman Creator** for high-quality, diverse characters
   - Alternative: Mixamo characters for lighter weight
   - Need 8-12 base variations (age, gender, ethnicity, style)

2. **Animation System**
   - **Motion Capture Data:**
     - Mixamo: Free mocap library (walking, idle, talking)
     - Rokoko: Real-time mocap (if budget allows)
     - Move.ai: AI-powered mocap from video
   - **Animation Blueprint:**
     - Blend space for walk speeds
     - State machine: Idle → Walk → Run → Special actions
     - IK for feet placement on uneven terrain

3. **AI Behaviors** (Mass AI Processors)
   ```cpp
   // Suggested Mass processors:
   - MassPedestrianWalkProcessor: Path following
   - MassPedestrianAvoidanceProcessor: RVO collision
   - MassPedestrianLODProcessor: Switch detail levels
   - MassPedestrianReactionProcessor: React to player/vehicles
   ```

4. **Performance Optimization**
   - **LOD System:**
     - LOD 0 (< 10m): Full animation, IK, facial
     - LOD 1 (10-30m): Simplified animation
     - LOD 2 (30-100m): Animated imposters
     - LOD 3 (> 100m): Static billboards
   - **Culling:** Only animate visible agents
   - **Pooling:** Reuse actors beyond spawn radius

### Current Code Migration Path

**From** `SprawlPedestrian.cpp`:
```cpp
// Current simple system
void Tick(float DeltaSeconds) {
    AddMovementInput(WanderDir, 1.f);
    // Simple direction changes
}
```

**To** Mass Entity approach:
```cpp
// Mass processor handles bulk updates
UMassPedestrianProcessor::Execute(FMassEntityManager& EntityManager,
                                   FMassExecutionContext& Context)
{
    // Process thousands of entities efficiently
    // Use entity fragments for position, velocity, state
}
```

---

## 🚗 Vehicle System Improvements

### Wheel Animation System

**Problem:** Current wheels are static meshes with no rotation

**Solution:** Chaos Vehicle Plugin + Animation Blueprint

#### Implementation Steps

1. **Enable Chaos Vehicles Plugin**
   - Edit → Plugins → Enable "Chaos Vehicles"

2. **Vehicle Setup**
   ```cpp
   // Replace ASprawlCar inheritance
   class ASprawlCar : public AWheeledVehiclePawn // Instead of APawn
   {
       // Add UChaosWheeledVehicleMovementComponent
       UPROPERTY()
       UChaosWheeledVehicleMovementComponent* VehicleMovement;
   };
   ```

3. **Wheel Blueprints**
   - Create `BP_SprawlWheelFront` and `BP_SprawlWheelRear`
   - Configure:
     - Radius: 41cm (matches current scale)
     - Friction: 3.0 (asphalt)
     - Suspension: Natural frequency 8Hz, damping 0.7

4. **Animation Blueprint for Wheels**
   ```
   AnimGraph:
   - WheelHandler node (automatic rotation based on physics)
   - Connects wheel bones to actual physics simulation
   ```

5. **Skeletal Mesh Requirements**
   - Vehicle mesh needs bones for each wheel
   - Bones named: `Wheel_FL`, `Wheel_FR`, `Wheel_RL`, `Wheel_RR`
   - Suspension bones optional but recommended

### Vehicle Assets

**Recommended Sources:**
- **Unreal Marketplace:**
  - "City Vehicles Pack" (realistic low-poly cars)
  - "Modular Street Vehicles" (customizable)
- **Quixel Megascans:**
  - High-quality vehicle scans (if performance allows)
- **Free Resources:**
  - Sketchfab (CC licensed models)
  - TurboSquid free section

**Specifications for Mobile (iPhone target):**
- Triangles: 5,000-15,000 per vehicle
- LOD0: Full detail for player's car
- LOD1: 50% tri count for nearby traffic
- LOD2: 25% tri count for distant traffic
- Materials: 2K textures max, 1K for traffic

### Physics Tuning

```cpp
// Realistic sedan values (in SprawlCar.cpp)
Mass: 1500kg                    // Current: 1500kg ✓
Engine Torque: 400 Nm @ 5000rpm
Max RPM: 6500
Drag Coefficient: 0.3           // Aerodynamic
Rolling Resistance: 0.015
Steering Angle: 35° (front wheels)
```

---

## 🏙️ World Environment Improvements

### Building System

**Current:** Simple geometry with basic materials
**Goal:** Detailed, varied, performant urban environment

#### Nanite for Buildings (UE5 Feature)

Nanite allows millions of polygons with no performance cost:
- Import high-detail building models
- Enable "Nanite" on static mesh
- No manual LOD creation needed
- Perfect for hero buildings in The Junction

**Implementation:**
1. Create/acquire detailed building meshes
2. Import to UE5
3. Enable Nanite in static mesh settings
4. Place in level - Nanite handles streaming/LOD

#### Building Variety System

```cpp
// Procedural building placement
class AZonalVisualsManager : public AActor
{
    // Current system can be extended:
    UPROPERTY()
    TArray<UStaticMesh*> JunctionBuildings;    // High-density

    UPROPERTY()
    TArray<UStaticMesh*> SuburbanBuildings;    // Low-rise

    UPROPERTY()
    TArray<UStaticMesh*> IndustrialBuildings;  // Warehouses

    void SpawnBuildingsForZone(FString ZoneName);
};
```

**Building Asset Packs (Unreal Marketplace):**
- "City Downtown Pack" - Skyscrapers and offices
- "Modular Neighborhood" - Residential buildings
- "Industrial Environment" - Warehouses for Rail Yards
- "Urban Props" - Street furniture and details

### Foliage System

**Unreal Foliage Tool** for grass, trees, shrubs:

1. **Asset Sources:**
   - **Quixel Megascans:** Photo-real vegetation (FREE with UE5)
     - Search: "grass", "tree", "bush", "shrub"
     - Types: Oak, Maple, Pine for urban areas
   - **SpeedTree:** Procedural trees with wind animation

2. **Foliage Types:**
   - **The Junction:** Street trees (London Plane, Oak)
   - **Parks:** Grass, flowers, benches
   - **Rail Yards:** Weeds, sparse vegetation
   - **Iron Forest:** Manicured lawns, ornamental trees

3. **Implementation:**
   ```
   Tools → Foliage → Add Foliage Type
   - Select mesh
   - Configure density (instances per 1000cm²)
   - Set scale variation (0.8-1.2)
   - Enable rotation variation
   - Paint onto landscape
   ```

4. **Performance:**
   - Use Hierarchical Instanced Static Meshes (HISM)
   - Cull distance: 0-5000cm based on detail level
   - Max draw distance for grass: 3000cm
   - Trees: 10000cm

### Lighting: Lumen Global Illumination

**Enable in Project Settings:**
- Rendering → Dynamic Global Illumination: Lumen
- Rendering → Reflections: Lumen

**Benefits:**
- Real-time bounce lighting
- Realistic ambient occlusion
- No need to bake lightmaps
- Perfect for day/night cycle

**Setup:**
```
1. Add Directional Light (Sun)
   - Intensity: 10 lux (day) / 0.1 lux (night)
   - Temperature: 5500K (day) / 2800K (sunset)

2. Add Sky Atmosphere
   - Automatic realistic sky

3. Add Exponential Height Fog
   - Fog Density: 0.02
   - Fog Height Falloff: 0.2

4. Post Process Volume
   - Unbound: True
   - Exposure: Auto
   - Bloom: Enabled
```

### Material System

**PBR Materials** for realism:

```cpp
// Material master for city surfaces
M_CityMaster:
- Base Color: RGB texture
- Normal: Normal map
- Roughness: 0.4-0.8 (varies by surface)
- Metallic: 0.0 (most surfaces) / 1.0 (metal)
- AO: Ambient occlusion map

// Material instances:
MI_Asphalt:    Roughness 0.8, subtle tire marks
MI_Concrete:   Roughness 0.6, weathering
MI_Brick:      Roughness 0.7, worn edges
MI_Glass:      Roughness 0.1, Metallic 0.3, opacity
```

**Texture Sources:**
- Quixel Megascans (free high-quality PBR)
- Polyhaven (free CC0 textures)
- Substance Source (subscription)

---

## 📊 Performance Targets (iPhone 13+)

### Mobile Optimization Strategy

**Current:** Forward rendering, Mobile HDR, static lighting
**Goal:** Maintain 60fps with realistic assets

#### Budget Breakdown

| System | Budget | Notes |
|--------|--------|-------|
| Pedestrians | 100-200 visible | Mass AI LOD system |
| Vehicles | 20-40 visible | Physics vehicles |
| Draw Calls | < 2000 | Instancing, Nanite |
| Triangles | 2-3M on screen | Nanite for buildings |
| Texture Memory | 500MB | Streaming, compression |

#### Optimization Techniques

1. **Level Streaming**
   ```cpp
   // Current ZonalStreamingManager extended:
   - Stream 1-mile radius (current) ✓
   - Add aggressive unloading beyond 1.5 miles
   - Preload along velocity vector (highway streaming)
   ```

2. **Asset LODs**
   - Buildings: Nanite (automatic)
   - Vehicles: 3 LODs (15k → 7k → 2k tris)
   - Pedestrians: 4 LODs including imposters
   - Props: 2-3 LODs

3. **Texture Streaming**
   - Enable Virtual Textures for terrain
   - Compress with ASTC (mobile)
   - Mip streaming budget: 200MB

4. **Rendering**
   - Keep Forward rendering ✓
   - Use Mobile HDR ✓
   - Limit dynamic lights: 3 max per object
   - Shadow distance: 2000cm

---

## 🛠️ Implementation Phases

### Phase 1: Pedestrians (1-2 agents)
**Tasks:**
- [ ] Research and acquire 3-5 MetaHuman characters
- [ ] Set up basic Mass Entity system
- [ ] Implement walk animation blend space
- [ ] Create pedestrian spawner with Mass
- [ ] Add simple avoidance behaviors
- [ ] Test performance with 100+ pedestrians

**Files to modify:**
- `SprawlPedestrian.cpp/.h` → Migrate to Mass Entity fragments
- Create `MassPedestrianProcessors.cpp`
- Create Animation Blueprint for pedestrians

### Phase 2: Vehicles (1-2 agents)
**Tasks:**
- [ ] Enable Chaos Vehicles plugin
- [ ] Acquire 3-5 vehicle meshes with wheel bones
- [ ] Create wheel blueprints (front/rear)
- [ ] Set up vehicle animation blueprint
- [ ] Refactor SprawlCar to use Chaos Vehicle
- [ ] Implement wheel rotation animation
- [ ] Add suspension visual feedback
- [ ] Test physics and handling

**Files to modify:**
- `SprawlCar.cpp/.h` → Inherit from AWheeledVehiclePawn
- `MobileOfficeVehicle.cpp/.h` → Update for new base class
- Create `BP_VehicleWheel_Front/Rear`
- Create `ABP_VehicleAnimation`

### Phase 3: World Environment (1-2 agents)
**Tasks:**
- [ ] Acquire building asset packs
- [ ] Enable Nanite on hero buildings
- [ ] Set up modular building placement system
- [ ] Integrate Quixel Megascans vegetation
- [ ] Configure foliage system with grass/trees
- [ ] Set up Lumen lighting
- [ ] Create material library (roads, buildings, nature)
- [ ] Add urban props (street lights, signs, benches)
- [ ] Optimize for mobile (LODs, culling)

**Files to modify:**
- `ZonalVisualsManager.cpp/.h` → Add building/foliage spawning
- Create material instances in Content/CityArt/
- Set up foliage types in Content/Foliage/

---

## 📚 Learning Resources

### Documentation
- [UE5 Mass Entity System](https://docs.unrealengine.com/5.0/en-US/mass-entity-in-unreal-engine/)
- [Chaos Vehicles](https://docs.unrealengine.com/5.0/en-US/chaos-vehicles-overview-in-unreal-engine/)
- [Nanite](https://docs.unrealengine.com/5.0/en-US/nanite-virtualized-geometry-in-unreal-engine/)
- [Lumen](https://docs.unrealengine.com/5.0/en-US/lumen-global-illumination-and-reflections-in-unreal-engine/)

### Sample Projects
- **City Sample** (Epic Games) - Free UE5 demo showcasing:
  - Mass Entity crowds
  - Nanite buildings
  - Lumen lighting
  - Vehicle traffic
  - Available: Unreal Marketplace

### Video Tutorials
- "UE5 Mass AI Tutorial" (YouTube)
- "Chaos Vehicle Setup" (YouTube)
- "Creating a Complete GTA Game in UE5" (Gorka Games)
- "Nanite Best Practices" (Unreal Engine channel)

---

## 🔄 Migration Strategy

### Gradual Upgrade Path

**Advantage:** Don't break existing gameplay systems

1. **Add alongside current systems**
   - New Mass pedestrians spawn separately from old SprawlPedestrians
   - New Chaos vehicles coexist with current SprawlCars
   - Compare performance and quality

2. **Parallel testing**
   - Toggle between old/new systems with Blueprint switch
   - Performance profiler comparisons
   - Visual quality comparisons

3. **Phased rollout**
   - Enable new system in one zone (e.g., The Junction only)
   - Once stable, expand to other zones
   - Remove old system last

### Backward Compatibility

Keep current systems functional:
```cpp
// Configuration flag
UPROPERTY(Config, EditAnywhere)
bool bUseLegacyPedestrians = false;

UPROPERTY(Config, EditAnywhere)
bool bUseLegacyVehicles = false;

// Spawn appropriate type based on flag
```

---

## 💡 Quick Wins (Low Effort, High Impact)

While planning major refactors, these can be done quickly:

1. **Pedestrian variety** (1 hour)
   - Download 3 free Mixamo characters
   - Swap out UE4 Mannequin
   - Randomize on spawn

2. **Vehicle paint variety** (30 min)
   - Already supported in SprawlCar!
   - Just expand the material array
   - Random on traffic spawn

3. **Basic foliage** (2 hours)
   - Download 5 Megascans plants
   - Paint grass along sidewalks
   - Place trees in strategic spots

4. **Improved materials** (1 hour)
   - Download Megascans asphalt/concrete
   - Replace current simple materials
   - Instant visual upgrade

5. **Wheel rotation shader trick** (1 hour)
   - Use material animation for fake rotation
   - Scroll tire texture based on velocity
   - Not physics-accurate but looks decent

---

## 🎯 Success Metrics

### Visual Quality
- [ ] Pedestrians indistinguishable from AAA games at 10m distance
- [ ] Vehicle wheels rotate in sync with speed
- [ ] Buildings have proper depth and detail
- [ ] Vegetation looks natural and varied
- [ ] Lighting feels realistic (time of day)

### Performance
- [ ] Maintain 60fps on iPhone 13+
- [ ] 100+ visible pedestrians
- [ ] 30+ visible vehicles
- [ ] Smooth streaming between zones
- [ ] < 2 second load times for zone transitions

### Gameplay Feel
- [ ] City feels alive and populated
- [ ] Traffic behaves believably
- [ ] Pedestrians react to player
- [ ] World has visual variety
- [ ] Maintains GDD's "Brooklyn-dense" vibe in The Junction

---

## 📝 Notes & Decisions

### Technical Decisions
- **Why Mass Entity vs traditional AI?**
  - Need 100+ pedestrians for city feel
  - Traditional AI too expensive (CPU)
  - Mass AI data-oriented, scales better

- **Why Chaos Vehicles?**
  - Modern UE5 standard
  - Better physics simulation
  - Built-in wheel animation
  - Replaces deprecated WheeledVehicle

- **Why Nanite?**
  - Target is iPhone 13+ (A15 chip supports)
  - Allows detailed buildings with no performance cost
  - Eliminates LOD creation work

### Design Decisions
- **Character realism level?**
  - Target: Stylized-realistic (not uncanny valley)
  - MetaHuman quality for Zarri
  - Simplified MetaHumans for pedestrians

- **Vehicle variety?**
  - Need 8-12 types minimum
  - Match zones: Junction = sedans, Rail Yards = trucks
  - Player car (Mobile Office) can be more detailed

- **World density?**
  - The Junction: Dense (per GDD)
  - Outskirts: Sparse
  - Use streaming to maintain density only where needed

---

## 🚀 Next Steps

1. **Prioritize** which phase to tackle first
2. **Assign agents** for parallel work if available
3. **Set up test map** for isolated feature testing
4. **Acquire assets** (MetaHumans, vehicles, buildings)
5. **Prototype** one feature fully before expanding
6. **Measure performance** continuously during development

---

## 📎 Asset Shopping List

### Characters
- [ ] 5 MetaHuman presets (diverse ages, genders)
- [ ] Mixamo animation pack (backup/lightweight option)

### Vehicles
- [ ] 3 sedan models (low, mid, high-end)
- [ ] 2 SUV models
- [ ] 1 truck model
- [ ] 1 sports car
- [ ] 1 beater (old car for Rail Yards)

### Environment
- [ ] Urban building pack (20+ buildings)
- [ ] Street props pack (lights, signs, benches)
- [ ] Megascans vegetation (10+ types)
- [ ] Road material pack
- [ ] Sky/weather system

### Sounds (future phase)
- [ ] Footstep sounds (concrete, asphalt, grass)
- [ ] Vehicle engine sounds
- [ ] Ambient city sounds

---

**Last Updated:** 2026-05-31
**Next Review:** After Phase 1 completion
