## CatSoup Adventure

**CatSoup Adventure** és un joc narratiu en Unreal Engine 5.7 ambientat en una presó medieval.  
El jugador explora, interactua amb l’entorn i conversa amb NPCs per avançar en la fugida, incloent un minijoc de *lockpicking* integrat a la UI.

## Estat dels requisits (rúbrica)

| Requisit | Estat | Implementació |
|---|---|---|
| (2p) Moviment i interacció del jugador | Implementat | Control de jugador amb moviment, càmera i sistema d’interacció amb objectes/NPC. |
| (2p) Controlador d’animacions | Implementat | Locomoció bàsica, bloqueig de moviment durant diàlegs i ús de montage/canvi de pose en context narratiu. |
| (2p) Minijoc UI complet (REF002) | Implementat | Minijoc de *lockpicking* completament en UI, connectat al flux del diàleg i als resultats de partida. |
| (1p) Menús | Implementat | Menú principal amb iniciar partida i sortir; menú de pausa que atura l’execució del joc. |
| (2p) UI de diàlegs (REF001) | Implementat | El widget de diàleg s’obre en iniciar conversa i es tanca en finalitzar-la. |
| (1p) Extres | Implementat | Build del projecte, nivell decorat i almenys 2 animacions d’UI. |
| (+1p) Joc en dos idiomes | Implementat | Localització en **anglès** i **espanyol** (textos de menús i diàlegs). |
| (+1p) Veus en diversos idiomes | Implementat | Veus/dialogues en múltiples idiomes amb integració d’àudio (FMOD). |

## Aspectes tècnics destacats

- Desenvolupament híbrid amb **Blueprints + C++**.
- Sistema de diàleg amb estructura de nodes, opcions i accions de joc.
- Integració d’àudio amb **FMOD** (música, FX i veus).
- Sistema de localització preparat per a múltiples idiomes.
- Estructura modular per escalar noves escenes, diàlegs i minijocs.

## Contingut principal del projecte

- **Gameplay**: exploració, interacció i progrés per objectius.
- **Narrativa**: converses amb opcions i conseqüències.
- **UI**: menús, pausa, diàlegs i minijoc.
- **Art/Level Design**: ambient de dungeon decorat per reforçar la immersió.

## Estat del projecte

El projecte es troba en estat jugable amb les funcionalitats principals i extres implementats segons la rúbrica, incloent els punts addicionals de localització i veus en més d’un idioma.
