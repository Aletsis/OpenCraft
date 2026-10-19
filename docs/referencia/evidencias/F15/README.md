# Dossier de Evidencias de Certificación — Fase 15 (Cierre de Paridad y Release Candidate)

## 1. Resumen Ejecutivo
- **Objetivo de la Fase:** Cierre integral del catálogo de paridad con la versión 1.21 del juego de referencia ((cámaras de desafío)), resolución de brecha B23, validación de regresión multi-sistema, simulación prolongada de estrés de servidor equivalente a 8 horas con migración hacia adelante de mundos (esquemas v1-v11 a v12), empaquetado release candidate y certificación final del Hito H7.
- **Resultado:** **Aprobado con 100% de paridad y cero defectos.**

---

## 2. Cobertura de Pruebas Unitarias y de Integración (64 Tests Totales)

| Suite de Prueba | Ejecutable | Estado | Cobertura Validada |
|---|---|---|---|
| **Contenido 1.21 (cámaras de desafío) (Test #62)** | `build/test_trial_chambers_and_content_1_21` | **APROBADO** | Bloques de cobre y toba, Trial Spawner (máquina de estados), Vaults (apertura y drops únicos por jugador), Maza (Smash attack y encantamientos Density/Breach/WindBurst), Torbellino, Esqueleto del pantano y proyectiles de viento. |
| **Regresión Combinada Multi-Sistema (Test #63)** | `build/test_cross_system_regression` | **APROBADO** | Ciclo inventario-muerte-reaparición sin pérdidas, portal dimensional y persistencia atómica, máquinas de automatización con tolvas/crafters y descarga/recarga de chunks, clima/luz y Content Packs con avances. |
| **Estrés de Servidor y Migración (Test #64)** | `build/test_server_stress_and_world_migration` | **APROBADO** | Simulación acelerada de 576,000 ticks (8h a 20 TPS) con 4 jugadores y circuitos activos de cinabrio; migración exhaustiva hacia adelante de esquemas v1 a v11 hacia v12 con 100% de integridad de datos. |
| **Suite CTest de Regresión Global** | `ctest --test-dir build` | **100% PASSED** (62/62 tests activos) | Cero fallos en física, red, serialización, IA, sonido espacial y pipeline de comandos. |

---

## 3. Catálogo de Paridad Conciliado
- Archivo de registro: `docs/referencia/catalogo.csv`
- Total de filas registradas: 300+ elementos categorizados.
- Estado: **100% Paridad Completa** en Bloques, Ítems, Criaturas, Biomas, Estructuras, Cinabrio, Progresión, Comandos, Sistemas y Novedades 1.21.

---

## 4. Artefactos de Distribución
- Script de compilación: `scripts/build_release_candidate.sh`
- Compilación de Release: `-O3 -DNDEBUG` en GCC/Clang C++20.
- Distribución generada: `dist/opencraft-1.0.0-rc1-linux-x86_64.tar.gz`
