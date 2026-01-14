# projekt_bildsynthese
Anleitung zum Starten des Projekts:
    Bauen mit dem Makefile -> Starten mit ./projekt

Beschreibung der Szene:


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
Deferred Shading        Objekte xy werden deferred gerendert, shaders/depth_only, gbuffer & lighting Shader, RenderPass & GraphicsPipeline
Spiegelung der Szene    
Compute Shader          In Szene für Schnee verwendet, helper/Compute/Snow, snow-Shader, Erstellung in main.cpp
Stencil Buffer          In Szene für Spiegel verwendet, helper/Mirrorsystem, Erstellung in main.cpp