# projekt_bildsynthese
Anleitung zum Starten des Projekts:
    Bauen mit dem Makefile -> Starten mit ./projekt

Beschreibung der Szene:
Die Zwergen-Brüder hatten mit ihrem Bus einen Unfall. Während sie jetzt auf den
Abschleppdienst warten, genießen sie das kühlste Bier der ganzen Wüste und betreiben Vandalismus.


Steuerung in der Szene:

W - Vorwärts
A - Links
S - Zurück
D - Rechts
Space - Hoch
Shift - Runter
Q - Geschwindigkeit * 5
Esc - Cursor vom Fenster lösen

Anforderung            Wo zu finden?

CubeMap                 helper/Texture/CubeMap, skybox-Shader, Erstellung in main.cpp
MipMaps                 helper/Texture/Texture 
Mehrere Pipelines       ObjectFactory erzeugt für jedes Objekt eine Pipeline
Deferred Shading        Objekte Der Linke Zwerg, der Regenschirm, der Schneehaufen, das Bier, der Bus, die Absperrung, der Stromkasten und die Grafittidosen werden deferred gerendert, shaders/depth_only, gbuffer & lighting Shader, RenderPass & GraphicsPipeline
Spiegelung der Szene    Die "magische" Kugel im Kaktuskreis verwendet render-to-texture helper/renderToTexture/..
Compute Shader          In Szene für Schnee verwendet, helper/Compute/Snow, snow-Shader, Erstellung in main.cpp
Stencil Buffer          In Szene für Spiegel verwendet, helper/Mirrorsystem, Erstellung in main.cpp


Quellen:

Regenschirm: https://free3d.com/3d-model/beach-umbrella-v1--514487.html

Gartenstuhl https://polyhaven.com/a/plastic_monobloc_chair_01

Gartenzwerg https://polyhaven.com/a/garden_gnome

Glühbirne: https://polyhaven.com/a/lightbulb_led

Tisch: https://polyhaven.com/a/chinese_tea_table

Stromkasten (?) : https://polyhaven.com/a/utility_box_01

Sprühdosen : https://polyhaven.com/a/spray_paint_bottles_02

Barrier: https://polyhaven.com/a/concrete_road_barrier_02

Fliege: https://free3d.com/3d-model/horse-fly-v1--357256.html

Straße: https://free3d.com/3d-model/street-estrada-971348.html

Wüste: https://sketchfab.com/3d-models/desert-landscape-220c14d161e44e83be64f30f2034cf4b

Schneehaufen: https://polyhaven.com/a/tree_stump_01

Bus: https://free3d.com/3d-model/tourist-bus-with-open-top-v2--502214.html

Bier: https://free3d.com/3d-model/-oz-beer-bottle-v1--386973.html

Kaktus: https://free3d.com/3d-model/-cactus-v1--424886.html