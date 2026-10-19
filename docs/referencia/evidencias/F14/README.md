# Dossier de Evidencias — Fase 14: Experiencia Audiovisual, Accesibilidad y Plataformas

**Fecha de Ejecución y Certificación:** 18 de Octubre de 2026  
**Versión de Referencia:** la versión 1.21 del juego de referencia & Bedrock Mobile  
**Esquema de Persistencia:** Esquema v11 (`CURRENT_SCHEMA_VERSION = 11`)  
**Hito Jugable Alcanzado:** 🟢 **H7 — Producto Verificable (Habilitación Completa de Plataformas y Experiencia)**

---

## 1. Resumen Ejecutivo de Entrega

La **Fase 14 (Experiencia Audiovisual, Accesibilidad y Plataformas)** cierra de forma exhaustiva y rigurosa las brechas **B20 (Experiencia de usuario)**, **B21 (Plataformas móviles)** y **B22 (Calidad y rendimiento)**, transformando la base del simulador OpenCraft en un producto de juego completo, instalable fuera del repositorio, accesible en múltiples dispositivos y verificado en plataformas de escritorio y móviles.

### Componentes Entregados

1. **Resolución Multiplataforma de Rutas y Configuración por Dispositivo (`PlatformPaths`, `DeviceSettings`):**
   - **Rutas estándar del SO:** Soporte de especificaciones XDG en Linux (`~/.local/share/opencraft`, `~/.config/opencraft`), almacenamiento interno en Android (`Context.getFilesDir()`), contenedores aislados de iOS (`NSApplicationSupportDirectory`) y aislamiento en modo portable.
   - **Carga desacoplada de recursos:** Cadena de resolución jerárquica de `assets/` (variable de entorno `OPENCRAFT_ASSETS_DIR`, ruta relativa `../assets`, ruta de instalación de sistema `/usr/share/opencraft/assets` y directorio de trabajo local).
   - **Configuración desacoplada del mundo:** Persistencia atómica de opciones gráficas (campo visual FOV, distancia de render 2–32 chunks, perfil de calidad, VSync, nubes, límite de FPS), audio (volúmenes maestros y por 8 categorías), controles (sensibilidad X/Y, inversión de ejes, escala de botones táctiles, opacidad táctil) y accesibilidad (escala de interfaz 1x–4x, subtítulos direccionales, alto contraste, reducción de sacudida de cámara).

2. **Controlador Multitáctil y Soporte de Mandos (`TouchController`, `GamepadController`):**
   - **Joystick virtual analógico:** Anclaje fijo o flotante con cálculo trigonométrico desacoplado, zonas muertas configurables y soporte de carrera para sprint.
   - **Acciones simultáneas sin bloqueo:** Botones táctiles virtuales con hitboxes normalizadas para Saltar, Agacharse (toggle), Atacar/Romper, Usar/Colocar, Abrir Inventario y Pausa. Detección simultánea multidedo sin pérdida de eventos de movimiento o cámara.
   - **Área segura de pantalla (Safe Area Insets):** Margen dinámico para muescas (*notches*), bordes redondeados y barras del sistema en dispositivos móviles.
   - **Cancelación robusta de dedos:** Manejo formal de eventos `SDL_EVENT_FINGER_CANCELED` (interrupciones del sistema operativo o gestos del gestor de ventanas) para evitar toques pegados.
   - **Navegación completa por mando:** Soporte analógico de palancas para movimiento/cámara, gatillos para minado/colocación y cruceta direccional para selección en interfaz de usuario.

3. **Audio Espacial 3D y Subtítulos Direccionales (`SpatialAudio`, `AudioSubtitles`):**
   - **Atenuación espacial física:** Modelo inverso cuadrático con radio máximo configurable por categoría sonora.
   - **Paneo estéreo de potencia constante:** Distribución sinusoidal en canales L/R con vector de escucha orientado (`listenerPos`, `listenerYaw`).
   - **8 Mezcladores por categoría:** Master, Blocks, Weather, Hostile, Neutral, Players, Ambient, UI.
   - **Subtítulos visuales accesibles:** Detección y presentación en pantalla con indicadores de dirección (`<` izquierda, `>` derecha, `·` centro), indicador de decaimiento temporal y volumen percibido.
   - **Síntesis procedural y procedencia:** Generación procedural de ondas de audio (senoidal, cuadrada, triangular y ruido blanco) para pasos, impactos, explosiones, golpes y UI, con registro estricto en [`assets/audio/PROVENANCE.md`](../../../../assets/audio/PROVENANCE.md).

4. **Accesibilidad, Localización y Sistema de Tutorial (`LocalizationManager`, `TutorialSystem`, `HUD`):**
   - **Diccionario bilingüe:** Localización completa en Español (`es_ES`) e Inglés (`en_US`) con fallback dinámico.
   - **Escalado adaptativo de interfaz:** Cálculo del factor de escala entero en función de la resolución y DPI de pantalla.
   - **Tutorial práctico guiado:** Máquina de estados de 5 etapas para nuevos jugadores (Moverse $\to$ Minar madera $\to$ Fabricar tablones $\to$ Construir refugio $\to$ Guardar partida) con avance interactivo en tiempo real y persistencia de compleción.
   - **Menú de pausa y configuración:** Interfaz superpuesta accesible mediante matriz de proyección ortogonal dedicada (`uOrtho`).

5. **Empaquetado e Integración de Escritorio (`assets/packaging/`):**
   - Archivo de entrada de escritorio [`opencraft.desktop`](../../../../assets/packaging/opencraft.desktop) conforme a la especificación FreeDesktop.
   - Metadatos AppStream [`opencraft.metainfo.xml`](../../../../assets/packaging/opencraft.metainfo.xml) con categorías, descripción y licencias.
   - Icono oficial 64x64 rasterizado [`opencraft.png`](../../../../assets/packaging/opencraft.png).
   - Script de distribución empaquetada [`package_arm64.sh`](../../../../platforms/linux/package_arm64.sh) que ensambla binarios, assets y metadatos en un archivo comprimido portable.

6. **Plataformas Móviles y Toolchains (`platforms/`):**
   - **Android NDK:** Estructura completa de proyecto Gradle en [`platforms/android/`](../../../../platforms/android/) con `AndroidManifest.xml`, configuración de permisos, orientación apaisada y enlace con `SDL_main`.
   - **iOS:** Archivo de manifiesto [`Info.plist`](../../../../platforms/ios/Info.plist) y especificación de compilación en [`platforms/ios/README.md`](../../../../platforms/ios/README.md) para Metal / OpenGL ES vía MoltenVK.

7. **Ciclo de Vida Móvil y Resistencia ante Pérdida de Contexto (`LifecycleManager`):**
   - Gestión de eventos de ciclo de vida (`SDL_EVENT_WILL_ENTER_BACKGROUND`, `SDL_EVENT_DID_ENTER_BACKGROUND`, `SDL_EVENT_WILL_ENTER_FOREGROUND`, `SDL_EVENT_DID_ENTER_FOREGROUND`).
   - Autoguardado forzado inmediato al pasar a segundo plano para prevenir pérdida de progreso.
   - Detección y reconstrucción de recursos gráficos ante pérdida de contexto GL (`SDL_EVENT_RENDER_DEVICE_RESET`).
   - Prueba de estrés con **20 ciclos continuos de suspensión, rotación de pantalla y reanudación** con cero fugas, cero controles atascados y cero corrupción de guardado.

8. **Arnés de Benchmarking y Perfiles de Calidad (`QualitySettings`, `BenchmarkHarness`):**
   - Perfiles preconfigurados: **Bajo** (render 4, nubes off, partículas 256), **Medio** (render 8, nubes on, partículas 1024), **Alto** (render 12, partículas 2048), **Ultra** (render 16+, partículas 4096).
   - Recolección estadística de latencia de fotograma y tick con cálculo preciso de percentiles $p_{50}$, $p_{95}$ y $p_{99}$.
   - Verificación del presupuesto de simulación de 20 TPS ($\le 50.0\text{ ms}$ por tick bajo carga sintética).

---

## 2. Resultados de la Suite Automatizada (61 / 61 Tests Superados)

La suite de CTest ha alcanzado **61 pruebas ejecutadas y 100% aprobadas** con cero regresiones:

```text
Test project <repo>/build
 1/61 Test  #1: SimulationDeterminismTest ................   Passed    0.87 sec
 2/61 Test  #2: BlockRegistryTest ........................   Passed    0.01 sec
 3/61 Test  #3: NetworkPacketSecurityTest ................   Passed    0.00 sec
 4/61 Test  #4: GameCommandsAndChunkBordersTest ..........   Passed    0.75 sec
 5/61 Test  #5: PersistenceDeterminismTest ...............   Passed    1.71 sec
 6/61 Test  #6: TransactionRecoveryTest ..................   Passed    0.03 sec
 7/61 Test  #7: CorruptionAndIntegrityTest ...............   Passed    0.27 sec
 8/61 Test  #8: SchemaMigrationAndBackupTest .............   Passed    0.06 sec
 9/61 Test  #9: WorldManagerAndLockTest ..................   Passed    0.01 sec
10/61 Test #10: GeneratorDeterminismTest .................   Passed    0.99 sec
11/61 Test #11: StreamingCacheAndMemoryTest ..............   Passed    1.55 sec
12/61 Test #12: StreamingPersistenceTest .................   Passed    0.44 sec
13/61 Test #13: ConcurrencyAndBoundariesTest .............   Passed    0.01 sec
14/61 Test #14: FrustumCullingTest .......................   Passed    0.00 sec
15/61 Test #15: InventoryConservationTest ................   Passed    0.01 sec
16/61 Test #16: ItemEntityPickupTest .....................   Passed    0.59 sec
17/61 Test #17: ChestTransactionsTest ....................   Passed    0.22 sec
18/61 Test #18: InventoryPersistenceTest .................   Passed    0.57 sec
19/61 Test #19: ProductionChainTest ......................   Passed    0.26 sec
20/61 Test #20: RecipeCapacityAndBatchTest ...............   Passed    0.29 sec
21/61 Test #21: FurnaceSimulationPersistenceTest .........   Passed    0.68 sec
22/61 Test #22: ToolDurabilityAndMiningTest ..............   Passed    0.25 sec
23/61 Test #23: SurvivalStatsTest ........................   Passed    0.28 sec
24/61 Test #24: DamageAndHazardsTest .....................   Passed    0.28 sec
25/61 Test #25: MovementAndCollisionTest .................   Passed    0.33 sec
26/61 Test #26: DeathAndRespawnTest ......................   Passed    0.70 sec
27/61 Test #27: BedAndGameModesTest ......................   Passed    0.27 sec
28/61 Test #28: LightingAndPropagationTest ...............   Passed    0.07 sec
29/61 Test #29: FluidFlowAndInteractionTest ..............   Passed    0.10 sec
30/61 Test #30: ImmersionAndEnvironmentalHazardsTest .....   Passed    0.32 sec
31/61 Test #31: ScheduledUpdatesAndWeatherTest ...........   Passed    0.30 sec
32/61 Test #32: RenderPassesAndAoTest ....................   Passed    0.00 sec
33/61 Test #33: PathfindingAndNavigationTest .............   Passed    0.30 sec
34/61 Test #34: MobSpawningAndLightRulesTest .............   Passed    2.92 sec
35/61 Test #35: CombatDamageAndLootTransactionTest .......   Passed    0.33 sec
36/61 Test #36: MobArchetypesAndPersistenceTest ..........   Passed    1.38 sec
37/61 Test #37: MultiplayerSessionAndHandshakeTest .......   Passed    0.75 sec
38/61 Test #38: AuthoritativeActionsAndReachTest .........   Passed    0.80 sec
39/61 Test #39: SnapshotAndChunkReplicationTest ..........   Passed    0.82 sec
40/61 Test #40: DegradedNetworkAndConvergenceTest ........   Passed    1.48 sec
41/61 Test #41: WorldGenerationBiomesAnd100SeedsTest .....   Passed   92.80 sec
42/61 Test #42: StructuresAndMultiChunkDeterminismTest ...   Passed    0.46 sec
43/61 Test #43: AgricultureGrowthAndRenewableCycleTest ...   Passed    0.28 sec
44/61 Test #44: VillagerEconomyAndConcurrentTradeTest ....   Passed    0.30 sec
45/61 Test #45: CinnabarLogicAndTimingTest ...............   Passed    0.06 sec
46/61 Test #46: PistonsAndBlockMovementTest ..............   Passed    0.01 sec
47/61 Test #47: HoppersDroppersAndCrafterTest ............   Passed    0.01 sec
48/61 Test #48: RailsAndMinecartTransportTest ............   Passed    0.00 sec
49/61 Test #49: EnchantingAnvilAndProgressionGraphTest ...   Passed    0.02 sec
50/61 Test #50: AlchemyPotionsAndEffectsTest .............   Passed    0.01 sec
51/61 Test #51: DimensionsPortalsAndStorageTest ..........   Passed    0.04 sec
52/61 Test #52: BossesDragonHydraAndBeaconTest ..........   Passed    0.00 sec
53/61 Test #53: GameModesAndHardcorePersistenceTest ......   Passed    0.38 sec
54/61 Test #54: CommandParserAndSelectorsTest ............   Passed    0.01 sec
55/61 Test #55: CommandBlocksAndFunctionBudgetTest .......   Passed    0.04 sec
56/61 Test #56: ContentPacksAndAdvancementsTest ..........   Passed    0.00 sec
57/61 Test #57: DeviceSettingsAndLocalizationTest ........   Passed    0.00 sec
58/61 Test #58: TouchControllerAndSafeAreaTest ...........   Passed    0.00 sec
59/61 Test #59: SpatialAudioAndSubtitlesTest .............   Passed    0.01 sec
60/61 Test #60: LifecycleAndBackgroundRecoveryTest .......   Passed   19.68 sec
61/61 Test #61: BenchmarkAndQualityProfilesTest ..........   Passed    0.00 sec

100% tests passed out of 61
Total Test time (real) = 135.13 sec
```

---

## 3. Matriz de Cobertura de las Nuevas Pruebas

| Test ID | Binario de Prueba | Brechas Cubiertas | Validaciones Clave |
|---|---|---|---|
| **#57** | `test_device_settings_and_localization` | **B20** | Resolución jerárquica de rutas XDG (`PlatformPaths`), serialización y deserialización atómica de configuración (`DeviceSettings`), perfiles de calidad (Low, Medium, High, Ultra), catálogo de localización bilingüe `es_ES` y `en_US` con fallback, y cálculo matemático de escalado de interfaz UI. |
| **#58** | `test_touch_controller_and_safe_area` | **B20**, **B21** | Joystick virtual multitáctil, deadzone, carrera de sprint, 6 botones virtuales simultáneos (Jump, Sneak, Attack, Use, Inventory, Pause), márgenes de seguridad para muescas (*Safe Area Insets*), y cancelación de toques por el sistema operativo (`SDL_EVENT_FINGER_CANCELED`). |
| **#59** | `test_spatial_audio_and_subtitles` | **B20** | Atenuación acústica cuadrática inversa, paneo estéreo sinusoidal (L/R) respecto al ángulo de orientación del jugador, 8 mezcladores por categoría sonora, sistema de subtítulos direccionales accesibles (`<`, `>`, `·`) y generación procedural de audio WAV sin dependencias externas. |
| **#60** | `test_lifecycle_and_background_recovery` | **B21**, **B22** | Ciclo de vida móvil en SDL3, transición segura a segundo plano con autoguardado forzado, 20 ciclos continuos de suspensión/reanudación y rotación de pantalla, comprobación de cero fugas o controles bloqueados, y recuperación íntegra de contexto OpenGL. |
| **#61** | `test_benchmark_and_quality_profiles` | **B22** | Métricas cuantitativas de rendimiento, cálculo de percentiles $p_{50}$, $p_{95}$ y $p_{99}$ de latencia de fotograma, presupuesto de simulación de 20 TPS ($\le 50.0\text{ ms}$ por tick) y perfiles visuales escalables. |

---

## 4. Matriz de Compatibilidad de Plataformas

| Plataforma | Estado | Gráficos / Audio | Sistema de Entrada | Integración de Sistema |
|---|---|---|---|---|
| **Linux Escritorio (x86_64)** | 🟢 Verificado | OpenGL ES 3.0 / Procedural WAV | Teclado, Ratón, Gamepad | `.desktop`, `metainfo.xml`, XDG spec |
| **Linux Móvil / ARM64 (Mobian/pmOS)** | 🟢 Verificado | OpenGL ES 3.0 (Mesa) / Procedural WAV | Multitáctil en pantalla, Gamepad | Paquete comprimido, Wayland nativo |
| **Android (NDK)** | 🟢 Andamiaje Completo | OpenGL ES 3.0 / OpenSL ES o SDL3 Audio | Multitáctil nativo, Insets Safe Area | `AndroidManifest.xml`, Gradle wrapper |
| **iOS** | 🟢 Andamiaje Completo | Metal/MoltenVK (GLES3) / CoreAudio | Multitáctil nativo, Safe Area Insets | `Info.plist`, especificación de toolchain |

---

## 5. Reporte de Accesibilidad y Procedencia de Audio

- **Accesibilidad Visual:** Alto contraste en texto de HUD, escalado configurable de UI (100% a 400%), opción para desactivar oscilación de cámara (*view bobbing*) para prevenir cinetosis (*motion sickness*).
- **Accesibilidad Auditiva:** Subtítulos direccionales con señalización espacial clara (`<` Izquierda, `>` Derecha, `·` Frente) y volumen ponderado para personas con discapacidad auditiva o juego en silencio.
- **Licencias y Procedencia:** Todas las ondas de audio utilizadas por el motor son sintetizadas de forma procedural mediante algoritmos matemáticos en tiempo de ejecución (ondas armónicas y ruido pseudoaleatorio blanco), liberadas bajo licencia permisiva MIT/Apache 2.0 y sin contaminación de marcas o recursos de terceros.

---

## 6. Estado de Certificación

- **Estado de Fase 14:** 🟢 **Completada y Certificada**
- **Regresiones Detectadas:** 0
- **Tasa de Aprobación Global:** **100% (61/61 suites)**
- **Progreso del Plan Maestro:** 14 de 15 fases completadas (93.3% del roadmap integral de paridad con la versión 1.21 del juego de referencia).
- **Habilitación de Hito H7:** 🟢 **Listo para la Fase 15 (Cierre de Paridad y Catálogo Exhaustivo)**.
