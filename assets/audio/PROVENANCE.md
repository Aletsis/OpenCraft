# Registro de Procedencia de Recursos de Audio — OpenCraft

**Proyecto:** OpenCraft (Sandbox Vóxel C++20)  
**Fase:** Fase 14 — Experiencia Audiovisual, Accesibilidad y Plataformas  
**Licencia de Audio:** MIT / CC0 (Dominio Público)  
**Autor:** OpenCraft Audio Core Generator

---

## 1. Declaración de Origen y Procedencia

Todos los efectos de sonido y formas de onda utilizados en OpenCraft son sintetizados procedimentalmente en tiempo de compilación/ejecución mediante el motor nativo `SpatialAudioEngine` o cargados desde fuentes con licencia permisiva CC0.

**No se utiliza ningún archivo binario propietario, extraído o licenciado bajo derechos de autor restrictivos pertenecientes a Mojang Studios, Microsoft ni terceros.**

---

## 2. Catálogo de Algoritmos de Síntesis Procedural

| Identificador de Sonido | Categoría Canónica | Algoritmo de Generación | Propósito / Evento |
|---|---|---|---|
| `step.generic` | Blocks | Ráfaga de ruido blanco estocástico (80 ms) con decaimiento cuadrático exponencial. | Sonido de pisadas sobre bloques sólidos (tierra, piedra). |
| `dig.generic` / `block.break` | Blocks | Ráfaga de ruido blanco filtrada (180 ms) con modulación de amplitud en peine. | Minado y ruptura de bloques vóxel. |
| `entity.generic.explode` | Hostile / Weather | Ruido pseudoaleatorio procesado mediante filtro paso bajo recursivo IIR de primer orden (850 ms, $\alpha=0.15$) con decaimiento $t^{1.8}$. | Detonaciones de TNT, acechadores y camas. |
| `entity.generic.splash` | Ambient / Players | Ruido filtrado con envolvente senoidal ($\sin(t \cdot \pi)$) y decaimiento armónico (350 ms). | Entrada y salida de cuerpos en agua y lava. |
| `entity.player.hurt` | Players | Onda senoidal con modulación de frecuencia descendente lineal (220 Hz $\to$ 110 Hz) en 220 ms. | Daño recibido por el jugador (caída, fuego, asfixia, ataque). |
| `ui.button.click` | Master | Pulso bifásico alternante de 20 ms con decaimiento rápido. | Interacción con botones de menú y slots de inventario. |
| `entity.player.levelup` | Players | Tono senoidal puro ascendente (440 Hz $\to$ 880 Hz) con reverberación simulada. | Subida de nivel de experiencia y obtención de logros. |

---

## 3. Especificaciones Técnicas del Motor de Audio

- **Frecuencia de Muestreo:** 44,100 Hz (44.1 kHz).
- **Profundidad de Bits:** 16-bit PCM lineal con signo Little-Endian.
- **Canales:** Mono espacializado en tiempo real a estéreo (panning estéreo angular y atenuación por distancia cuadrática).
- **Subtítulos:** Accesibilidad visual completa mediante `AudioSubtitleManager` con indicación de dirección angular (`<`, `>`, `·`).
- **Aislamiento en Pruebas:** Todos los generadores y el modelo de espacialización funcionan 100% en C++ puro sin requerir dispositivos de sonido hardware ni dependencias externas de códecs privativos.
