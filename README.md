# projekt_bildsynthese
## Anleitung zum Starten des Projekts:
    Bauen mit dem Makefile -> Starten mit ./projekt

## Beschreibung der Szene:
Die Zwergen-Brüder hatten mit ihrem Bus einen Unfall. Während sie jetzt auf den
Abschleppdienst warten, genießen sie das kühlste Bier der ganzen Wüste (welches ihnen der magische Fisch hergezaubert hat) und betreiben Vandalismus. Wir sind eine kleine Fliege, die durch das Geschehen schwirrt.

--- 

## Steuerung

| Taste | Funktion |
|-------|----------|
| **W** | Vorwärts |
| **A** | Links |
| **S** | Zurück |
| **D** | Rechts |
| **Space** | Hoch |
| **Shift** | Runter |
| **Q** | Geschwindigkeit × 5 |
| **Esc** | Cursor vom Fenster lösen |

---

## Implementierte Features

| Anforderung | Implementation |
|-------------|----------------|
| **CubeMap** | `helper/Texture/CubeMap`, skybox-Shader, Erstellung in `main.cpp` |
| **MipMaps** | `helper/Texture/Texture` |
| **Mehrere Pipelines** | ObjectFactory erzeugt für jedes Objekt eine eigene Pipeline |
| **Deferred Shading** | Linker Zwerg, Regenschirm, Schneehaufen, Bier, Bus, Absperrung, Stromkasten und Graffiti-Dosen<br>Shader: `shaders/depth_only`, `gbuffer` & `lighting`<br>Implementierung: `RenderPass` & `GraphicsPipeline` |
| **Spiegelung der Szene** | Magischer Fisch im Kaktuskreis verwendet Render-to-Texture<br>Implementierung: `helper/renderToTexture/` |
| **Compute Shader** | Schnee-Simulation<br>Implementierung: `helper/Compute/Snow`, snow-Shader, Erstellung in `main.cpp` |
| **Stencil Buffer** | Spiegel-System<br>Implementierung: `helper/MirrorSystem`, Erstellung in `main.cpp` |
| **Tessellation** | Die obere Kugel des Schneemanns ist tesselliert, die untere nicht (beides das gleiche Modell) <br>Implementierung: `shaders/tessellation`, Kleinigkeiten in `Frame.cpp` und `GraphicsPipeline.cpp` |

---

## Quellen

| Modell | Quelle |
|-------|--------|
| Regenschirm | [Free3D - Beach Umbrella](https://free3d.com/3d-model/beach-umbrella-v1--514487.html) |
| Gartenstuhl | [Poly Haven - Plastic Monobloc Chair](https://polyhaven.com/a/plastic_monobloc_chair_01) |
| Gartenzwerg | [Poly Haven - Garden Gnome](https://polyhaven.com/a/garden_gnome) |
| Glühbirne | [Poly Haven - LED Lightbulb](https://polyhaven.com/a/lightbulb_led) |
| Tisch | [Poly Haven - Chinese Tea Table](https://polyhaven.com/a/chinese_tea_table) |
| Fisch | [Free3D - Fish V1](https://free3d.com/3d-model/fish-v1--996288.html) |
| Pferd | [Free3D - Horse V01](https://free3d.com/3d-model/-horse-v01--801409.html) |
| Stromkasten | [Poly Haven - Utility Box](https://polyhaven.com/a/utility_box_01) |
| Sprühdosen | [Poly Haven - Spray Paint Bottles](https://polyhaven.com/a/spray_paint_bottles_02) |
| Absperrung | [Poly Haven - Concrete Road Barrier](https://polyhaven.com/a/concrete_road_barrier_02) |
| Fliege | [Free3D - Horse Fly](https://free3d.com/3d-model/horse-fly-v1--357256.html) |
| Straße | [Free3D - Street](https://free3d.com/3d-model/street-estrada-971348.html) |
| Wüste | [Sketchfab - Desert Landscape](https://sketchfab.com/3d-models/desert-landscape-220c14d161e44e83be64f30f2034cf4b) |
| Schneehaufen | [Poly Haven - Tree Stump](https://polyhaven.com/a/tree_stump_01) |
| Bus | [Free3D - Tourist Bus](https://free3d.com/3d-model/tourist-bus-with-open-top-v2--502214.html) |
| Bier | [Free3D - Beer Bottle](https://free3d.com/3d-model/-oz-beer-bottle-v1--386973.html) |
| Kaktus | [Free3D - Cactus](https://free3d.com/3d-model/-cactus-v1--424886.html) |
| Zauberhut | [Free3D - WizardHat V2](https://free3d.com/3d-model/wizardhat-v2--336163.html) |