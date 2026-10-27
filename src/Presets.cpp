#include "PanoramaDSP.h"
namespace panorama {
const std::array<Patch,kPresetCount>& factoryPresets(){
 static const std::array<Patch,kPresetCount> p=[] {
 auto v=std::array<Patch,kPresetCount>{{
 // name, engine, cutoff,res, A D S R, detune,unison,fm,fold,body,damp,motion,blend,bright,gain
 {"01 / Crystal Crown", Engine::CrystalVA,.84f,.15f,.015f,.23f,.76f,.48f,.29f,.93f,.08f,.12f,.2f,.42f,.28f,.5f,.86f,.51f},
 {"02 / Silver Pulse",Engine::CrystalVA,.67f,.37f,.008f,.30f,.50f,.22f,.12f,.31f,.10f,.05f,.25f,.51f,.18f,.52f,.48f,.67f},
 {"03 / Solar Glass",Engine::CrystalVA,.92f,.11f,.55f,1.40f,.78f,1.30f,.12f,.78f,.07f,.16f,.45f,.25f,.56f,.53f,.91f,.54f},
 {"04 / Nebula Bloom",Engine::FractalDigital,.78f,.22f,.48f,1.00f,.76f,1.55f,.12f,.19f,.45f,.39f,.48f,.44f,.78f,.61f,.75f,.58f},
 {"05 / Algorithmic Bell",Engine::FractalDigital,.97f,.12f,.003f,1.15f,.0f,1.20f,.05f,.06f,.78f,.63f,.42f,.27f,.20f,.52f,.89f,.45f},
 {"06 / Fractal Aurora",Engine::FractalDigital,.88f,.27f,.12f,.72f,.63f,1.0f,.21f,.51f,.68f,.81f,.4f,.36f,.95f,.48f,.67f,.49f},
 {"07 / Breathing Metal",Engine::PhysicalResonance,.75f,.15f,.05f,.80f,.49f,1.25f,.13f,.24f,.23f,.19f,.85f,.23f,.68f,.53f,.74f,.64f},
 {"08 / Glass Orchard",Engine::PhysicalResonance,.95f,.10f,.003f,.56f,.12f,1.65f,.05f,.15f,.30f,.13f,.90f,.16f,.32f,.34f,.94f,.65f},
 {"09 / Velvet Strings",Engine::PhysicalResonance,.66f,.24f,.32f,.68f,.71f,1.45f,.21f,.40f,.14f,.05f,.71f,.48f,.64f,.55f,.57f,.61f},
 {"10 / Astral Chimes",Engine::Hybrid,.95f,.17f,.003f,.90f,.26f,1.50f,.17f,.46f,.44f,.24f,.86f,.23f,.67f,.86f,.86f,.45f},
 {"11 / Living Supersaw",Engine::Hybrid,.84f,.29f,.08f,.43f,.73f,.70f,.33f,.90f,.16f,.14f,.72f,.45f,.55f,.42f,.80f,.48f},
 {"12 / Fractal Tide",Engine::Hybrid,.70f,.30f,.24f,.79f,.61f,1.70f,.21f,.55f,.62f,.66f,.78f,.52f,.89f,.73f,.65f,.46f},
 // v0.2: four original routing-specific sound-design studies
 {"13 / Solar Harp",Engine::Hybrid,.86f,.20f,.010f,.60f,.37f,1.10f,.26f,.80f,.09f,.18f,.85f,.25f,.26f,.88f,.84f,.48f,1,.83f,.86f},
 {"14 / Mercury Veil",Engine::Hybrid,.91f,.14f,.27f,.84f,.64f,1.75f,.18f,.28f,.73f,.65f,.87f,.37f,.80f,.87f,.77f,.46f,2,.88f,.82f},
 {"15 / Paradox Bells",Engine::Hybrid,.90f,.22f,.007f,.84f,.08f,2.10f,.17f,.49f,.76f,.48f,.94f,.19f,.37f,.96f,.95f,.42f,3,.94f,.94f},
 {"16 / Breathing Aurora",Engine::Hybrid,.73f,.25f,.32f,1.0f,.68f,1.50f,.32f,.81f,.48f,.42f,.90f,.53f,.76f,.65f,.71f,.52f,4,.84f,.96f},
 // v0.3: eight freely rewired modular patch studies (no samples, no source-code reuse).
 {"17 / Midnight Geometry",Engine::Modular,.81f,.18f,.015f,.64f,.72f,.8f,.42f,.8f,.57f,.61f,.79f,.31f,.40f,.53f,.92f,.48f},
 {"18 / Quartz Choir",Engine::Modular,.65f,.21f,.19f,1.02f,.69f,1.7f,.28f,.7f,.81f,.69f,.88f,.15f,.61f,.70f,.83f,.45f},
 {"19 / Glass Current",Engine::Modular,.77f,.19f,.003f,.68f,.30f,1.2f,.11f,.31f,.42f,.32f,.92f,.44f,.49f,.61f,.96f,.54f},
 {"20 / Cinder Orbit",Engine::Modular,.88f,.32f,.03f,.28f,.80f,.66f,.24f,.65f,.76f,.93f,.74f,.3f,.48f,.62f,.84f,.49f},
 {"21 / Lattice Rain",Engine::Modular,.76f,.20f,.015f,.64f,.29f,1.21f,.24f,.62f,.39f,.65f,.97f,.44f,.40f,.70f,.83f,.49f},
 {"22 / Electric Fossil",Engine::Modular,.74f,.27f,.07f,.53f,.71f,1.44f,.32f,.87f,.77f,.49f,.82f,.28f,.59f,.73f,.75f,.51f},
 {"23 / Static Cathedral",Engine::Modular,.91f,.14f,.72f,1.12f,.79f,2.12f,.08f,.21f,.88f,.46f,.86f,.35f,.92f,.62f,.82f,.45f},
 {"24 / Prismatic Organism",Engine::Modular,.95f,.12f,.02f,1.21f,.61f,1.49f,.28f,.68f,.73f,.74f,.96f,.14f,.71f,.84f,.97f,.50f},
 }};
 // v0.4: reimagined additive and phase-warp sound design; these are deliberately
 // not required to resemble the v0.1 VA/legacy factory sounds.
 v[24]={"25 / Midnight Geometry Reforged",Engine::Modular,.88f,.15f,.045f,.65f,.69f,1.1f,.23f,.55f,.59f,.52f,.83f,.26f,.52f,.53f,.84f,.48f};
 v[25]={"26 / Sine Cathedral",Engine::Modular,.70f,.08f,.34f,.92f,.70f,1.9f,.08f,.24f,.26f,.23f,.91f,.18f,.39f,.83f,.72f,.56f};
 v[26]={"27 / Prisma Engine",Engine::Modular,.89f,.12f,.004f,.47f,.67f,.55f,.18f,.46f,.67f,.38f,.80f,.42f,.61f,.67f,.87f,.48f};
 v[27]={"28 / Polar Machinery",Engine::Modular,.78f,.14f,.026f,.41f,.62f,.78f,.15f,.31f,.65f,.50f,.80f,.26f,.58f,.72f,.79f,.52f};
 v[28]={"29 / Glass Harmonics",Engine::Modular,.90f,.08f,.007f,.85f,.23f,1.55f,.16f,.57f,.25f,.52f,.94f,.19f,.43f,.92f,.83f,.48f};
 v[29]={"30 / Spectral Migration",Engine::Modular,.69f,.15f,.22f,.91f,.72f,1.7f,.25f,.64f,.72f,.65f,.77f,.38f,.69f,.84f,.66f,.49f};
 v[30]={"31 / Phase Orchid",Engine::Modular,.85f,.16f,.04f,.62f,.64f,1.14f,.23f,.52f,.48f,.43f,.83f,.32f,.59f,.84f,.77f,.53f};
 v[31]={"32 / Iron Nebula",Engine::Modular,.94f,.24f,.018f,.71f,.55f,.92f,.19f,.68f,.79f,.76f,.88f,.27f,.68f,.83f,.78f,.48f};
 // Patchbay assignments: independent nodes can be rewired freely; only graph cycles are illegal.
 auto set=[](Patch& p,int slot,NodeType type,int a=-1,int b=-1,float level=.8f,float color=.5f){
    p.graph.nodes[slot]={type,a,b,level,color};
 };
 // All eight designs have different topology, rather than just parameter changes.
 {
 auto&p=v[16];set(p,0,NodeType::VA,-1,-1,.94f,.92f);set(p,1,NodeType::Digital,-1,-1,.80f,.41f);
 set(p,2,NodeType::Mix,0,1,.84f,.36f);set(p,3,NodeType::Fold,2,-1,.75f,.42f);
 set(p,4,NodeType::Modal,3,-1,.73f,.68f);set(p,5,NodeType::LowPass,4,-1,.81f,.63f);
 set(p,6,NodeType::VA,-1,-1,.55f,.27f);set(p,7,NodeType::Mix,5,6,.78f,.19f);
 set(p,8,NodeType::Output,7,4,.86f,.2f);
 }
 {
 auto&p=v[17];set(p,0,NodeType::Digital,-1,-1,.89f,.65f);set(p,1,NodeType::Fold,0,-1,.71f,.23f);
 set(p,2,NodeType::Modal,1,-1,.90f,.85f);set(p,3,NodeType::VA,-1,-1,.48f,.16f);
 set(p,4,NodeType::LowPass,3,-1,.55f,.37f);set(p,5,NodeType::Mix,2,4,.80f,.21f);
 set(p,6,NodeType::Digital,2,-1,.31f,.18f);set(p,7,NodeType::Mix,5,6,.90f,.16f);
 set(p,8,NodeType::Output,7,2,.89f,.17f);
 }
 {
 auto&p=v[18];set(p,0,NodeType::Noise,-1,-1,.75f,.11f);set(p,1,NodeType::Digital,-1,-1,.67f,.38f);
 set(p,2,NodeType::Mix,0,1,.84f,.68f);set(p,3,NodeType::Modal,2,-1,.91f,.97f);
 set(p,4,NodeType::LowPass,3,-1,.82f,.67f);set(p,5,NodeType::VA,-1,-1,.67f,.55f);
 set(p,6,NodeType::Ring,4,5,.47f,.69f);set(p,7,NodeType::Mix,4,6,.84f,.15f);
 set(p,8,NodeType::Output,7,-1,.90f,0.f);
 }
 {
 auto&p=v[19];set(p,0,NodeType::VA,-1,-1,.90f,.84f);set(p,1,NodeType::Digital,-1,-1,.88f,.88f);
 set(p,2,NodeType::Ring,0,1,.92f,.36f);set(p,3,NodeType::Fold,2,-1,.88f,.72f);
 set(p,4,NodeType::Modal,3,-1,.76f,.57f);set(p,5,NodeType::LowPass,4,-1,.92f,.84f);
 set(p,6,NodeType::Gain,0,-1,.29f,.50f);set(p,7,NodeType::Mix,5,6,.88f,.17f);
 set(p,8,NodeType::Output,7,4,.85f,.10f);
 }
 {
 auto&p=v[20];set(p,0,NodeType::Noise,-1,-1,.79f,.02f);set(p,1,NodeType::VA,-1,-1,.86f,.48f);
 set(p,2,NodeType::Digital,1,-1,.84f,.67f);set(p,3,NodeType::Mix,0,2,.85f,.68f);
 set(p,4,NodeType::Modal,3,-1,.96f,.91f);set(p,5,NodeType::Fold,4,-1,.61f,.18f);
 set(p,6,NodeType::LowPass,5,-1,.82f,.79f);set(p,7,NodeType::Mix,4,6,.90f,.29f);
 set(p,8,NodeType::Output,7,1,.90f,.09f);
 }
 {
 auto&p=v[21];set(p,0,NodeType::VA,-1,-1,.94f,.75f);set(p,1,NodeType::Digital,0,-1,.89f,.44f);
 set(p,2,NodeType::Digital,-1,-1,.58f,.84f);set(p,3,NodeType::Ring,1,2,.79f,.45f);
 set(p,4,NodeType::Mix,0,3,.80f,.57f);set(p,5,NodeType::LowPass,4,-1,.88f,.38f);
 set(p,6,NodeType::Modal,5,-1,.81f,.54f);set(p,7,NodeType::Mix,5,6,.85f,.39f);
 set(p,8,NodeType::Output,7,1,.82f,.17f);
 }
 {
 auto&p=v[22];set(p,0,NodeType::Noise,-1,-1,.50f,.89f);set(p,1,NodeType::Digital,-1,-1,.77f,.87f);
 set(p,2,NodeType::Mix,0,1,.75f,.54f);set(p,3,NodeType::Fold,2,-1,.58f,.35f);
 set(p,4,NodeType::Modal,3,-1,.83f,.98f);set(p,5,NodeType::Modal,1,-1,.62f,.48f);
 set(p,6,NodeType::Mix,4,5,.80f,.37f);set(p,7,NodeType::LowPass,6,-1,.92f,.59f);
 set(p,8,NodeType::Output,7,-1,.88f,.0f);
 }
 {
 auto&p=v[23];set(p,0,NodeType::VA,-1,-1,.92f,.89f);set(p,1,NodeType::Digital,0,-1,.86f,.92f);
 set(p,2,NodeType::Ring,0,1,.76f,.5f);set(p,3,NodeType::Fold,2,-1,.65f,.53f);
 set(p,4,NodeType::Modal,3,-1,.92f,.78f);set(p,5,NodeType::LowPass,4,-1,.86f,.81f);
 set(p,6,NodeType::Mix,1,5,.85f,.45f);set(p,7,NodeType::Gain,6,-1,.87f,.50f);
 set(p,8,NodeType::Output,7,0,.81f,.12f);
 }
 // v0.4: additive harmonic generator (11), phase-warp oscillator (12), and
 // 2 cross-modulation lanes. Route IDs: Off0 LFO-A1 LFO-B2 Envelope3
 // Velocity4 Wheel5, Audio node K is K+6 (zero-based K).
 {
 auto&p=v[24]; // Same topology as Midnight Geometry, revoiced with a harmonic generator.
 p.graph=v[16].graph;
 set(p,0,NodeType::Harmonic,-1,-1,.94f,.47f);
 set(p,6,NodeType::PhaseWarp,-1,-1,.55f,.48f);
 p.modRoutes={ModRoute{1,0,0,.65f},ModRoute{7,3,0,.37f}};
 }
 {
 auto&p=v[25];set(p,0,NodeType::Harmonic,-1,-1,.93f,.76f);
 set(p,1,NodeType::PhaseWarp,-1,-1,.65f,.36f);set(p,2,NodeType::Mix,0,1,.80f,.27f);
 set(p,3,NodeType::Modal,2,-1,.88f,.87f);set(p,4,NodeType::Modal,0,-1,.63f,.53f);
 set(p,5,NodeType::Mix,3,4,.83f,.45f);set(p,6,NodeType::LowPass,5,-1,.83f,.66f);
 set(p,7,NodeType::Gain,6,-1,.92f,.50f);set(p,8,NodeType::Output,7,-1,.88f,0.f);
 p.modRoutes={ModRoute{1,0,0,.72f},ModRoute{3,1,2,.25f}};
 }
 {
 auto&p=v[26];set(p,0,NodeType::PhaseWarp,-1,-1,.95f,.45f);
 set(p,1,NodeType::Harmonic,-1,-1,.92f,.32f);set(p,2,NodeType::Ring,0,1,.77f,.52f);
 set(p,3,NodeType::Mix,0,2,.88f,.32f);set(p,4,NodeType::Fold,3,-1,.63f,.49f);
 set(p,5,NodeType::Modal,4,-1,.84f,.72f);set(p,6,NodeType::LowPass,5,-1,.83f,.60f);
 set(p,7,NodeType::Mix,3,6,.77f,.59f);set(p,8,NodeType::Output,7,-1,.88f,0.f);
 p.modRoutes={ModRoute{2,0,0,.54f},ModRoute{6,1,3,.21f}};
 }
 {
 auto&p=v[27];set(p,0,NodeType::Harmonic,-1,-1,.87f,.17f);
 set(p,1,NodeType::Digital,0,-1,.93f,.67f);set(p,2,NodeType::PhaseWarp,-1,-1,.80f,.79f);
 set(p,3,NodeType::Ring,1,2,.74f,.51f);set(p,4,NodeType::Mix,0,3,.85f,.35f);
 set(p,5,NodeType::Fold,4,-1,.60f,.48f);set(p,6,NodeType::Modal,5,-1,.78f,.51f);
 set(p,7,NodeType::Mix,6,4,.85f,.25f);set(p,8,NodeType::Output,7,-1,.87f,0.f);
 p.modRoutes={ModRoute{5,0,0,.40f},ModRoute{8,5,1,.55f}};
 }
 {
 auto&p=v[28];set(p,0,NodeType::Noise,-1,-1,.39f,.04f);
 set(p,1,NodeType::Harmonic,-1,-1,.96f,.12f);set(p,2,NodeType::Mix,0,1,.80f,.82f);
 set(p,3,NodeType::Modal,2,-1,.91f,.85f);set(p,4,NodeType::PhaseWarp,-1,-1,.50f,.49f);
 set(p,5,NodeType::Mix,3,4,.80f,.12f);set(p,6,NodeType::LowPass,5,-1,.85f,.79f);
 set(p,7,NodeType::Gain,6,-1,.87f,.48f);set(p,8,NodeType::Output,7,-1,.94f,0.f);
 p.modRoutes={ModRoute{2,1,0,.79f},ModRoute{3,3,1,.40f}};
 }
 {
 auto&p=v[29];set(p,0,NodeType::Harmonic,-1,-1,.90f,.80f);
 set(p,1,NodeType::PhaseWarp,-1,-1,.83f,.30f);set(p,2,NodeType::Digital,1,-1,.75f,.44f);
 set(p,3,NodeType::Mix,0,2,.87f,.51f);set(p,4,NodeType::Fold,3,-1,.66f,.28f);
 set(p,5,NodeType::Modal,4,-1,.88f,.89f);set(p,6,NodeType::LowPass,5,-1,.79f,.61f);
 set(p,7,NodeType::Mix,6,0,.89f,.11f);set(p,8,NodeType::Output,7,-1,.90f,0.f);
 p.modRoutes={ModRoute{1,0,0,-.72f},ModRoute{7,2,3,.47f}};
 }
 {
 auto&p=v[30];set(p,0,NodeType::PhaseWarp,-1,-1,.95f,.21f);
 set(p,1,NodeType::Harmonic,-1,-1,.81f,.91f);set(p,2,NodeType::Digital,0,-1,.82f,.40f);
 set(p,3,NodeType::Ring,1,2,.78f,.40f);set(p,4,NodeType::Mix,0,3,.87f,.36f);
 set(p,5,NodeType::Modal,4,-1,.79f,.79f);set(p,6,NodeType::LowPass,5,-1,.88f,.71f);
 set(p,7,NodeType::Mix,6,0,.86f,.13f);set(p,8,NodeType::Output,7,-1,.90f,0.f);
 p.modRoutes={ModRoute{2,0,0,.68f},ModRoute{3,2,3,.63f}};
 }
 {
 auto&p=v[31];set(p,0,NodeType::Harmonic,-1,-1,.94f,.39f);
 set(p,1,NodeType::Digital,0,-1,.91f,.69f);set(p,2,NodeType::Ring,0,1,.88f,.41f);
 set(p,3,NodeType::Fold,2,-1,.80f,.65f);set(p,4,NodeType::Modal,3,-1,.79f,.94f);
 set(p,5,NodeType::PhaseWarp,-1,-1,.67f,.85f);set(p,6,NodeType::Mix,4,5,.93f,.34f);
 set(p,7,NodeType::LowPass,6,-1,.82f,.76f);set(p,8,NodeType::Output,7,-1,.91f,0.f);
 p.modRoutes={ModRoute{1,5,0,.83f},ModRoute{6,1,3,.42f}};
 }
 return v;
 }();
 return p;
}
}
