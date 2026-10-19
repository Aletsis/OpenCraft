# Fase 13 — Modos completos, comandos y contenido extensible

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md) · [Dossier de Evidencias (F13)](../evidencias/F13/README.md)

Estado: 🟢 **Completada**. Dependencias de entrada: **F12**. Brechas cubiertas: **B14, B18, B19**. Esquema de persistencia: **v11**.

## Qué lograr, por qué y para qué

Cubrir juego libre, mapas de aventura y creación de contenido sin recompilar el motor. Las herramientas de administración también facilitan pruebas y mantenimiento.

## Alcance y límites

Creativo completo, Supervivencia, Aventura, Espectador, Extremo, reglas, comandos, avances y paquetes de datos/recursos propios. No garantiza compatibilidad con mods Java, plugins ni formatos del juego de referencia.

## Pasos de implementación

1. Completar políticas por jugador y por mundo: catálogo creativo buscable, vuelo, restricciones de aventura, espectador sin colisión/interacción, y muerte permanente en Extremo con transición definida.

2. Implementar parser de comandos, selectores acotados, permisos y ayuda. Priorizar modo, tiempo, clima, teleport, give, summon, setblock/fill y reglas; enumerar el resto en catálogo.

3. Agregar funciones y bloques de comandos bajo permisos, límites de operaciones y prevención de bucles. Todas las modificaciones pasan por autoridad y registran resultado.

4. Mover recetas, botín, avances y definiciones aptas a paquetes con manifest/versiones/dependencias. Separar recursos visuales de reglas de simulación; validar antes de activar y calcular hash para red.

5. Agregar personalización de avatar, idiomas y recursos de audio/modelos propios. Documentar prioridades al combinar paquetes, manejo de IDs faltantes y copia del mundo antes de cambios incompatibles.

6. Crear ejemplo de paquete que añada receta, variante visual y objetivo; poder habilitarlo/deshabilitarlo con diagnósticos claros. No ejecutar código nativo arbitrario como mecanismo inicial.

7. Completar estadísticas/avances y herramientas de generación de fixtures para la certificación F15.

## Entregables y estado al finalizar

Todos los modos accesibles, consola documentada, permisos, esquema de paquetes y ejemplos instalables. Matriz de comandos/avances pendientes frente a referencia.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Mismo mundo en distintos modos | Romper, usar, recibir daño, volar y morir | Cada modo respeta su política; un cliente no se concede permisos. |
| Paquete válido y otro con referencias rotas | Activarlos y conectar clientes con distinto hash | Carga válida o error accionable; incompatibilidad detectada antes de jugar. |
| Función recursiva y fill excesivo | Ejecutar con usuario normal y administrador | Permisos y presupuesto impiden bloquear servidor; cancelación recuperable. |
| Mundo Extremo después de morir | Cerrar y volver a abrir sin editar archivos | La restricción persiste y no restaura supervivencia por reinicio. |

Evidencia completa y certificada documentada en [Dossier de Evidencias de la Fase 13](../evidencias/F13/README.md) (56/56 pruebas automatizadas aprobadas al 100%).

## Riesgos y decisiones técnicas

Riesgo: prometer ecosistema compatible sin implementarlo. Los formatos OpenCraft se documentan como propios; importadores y protocolos externos quedan como proyectos adicionales. Los comandos no deben saltarse invariantes de guardado.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias (`test_gamemodes_and_hardcore_persistence`, `test_command_parser_and_selectors`, `test_command_blocks_and_function_budget`, `test_content_packs_and_advancements`).
- [x] La regresión de fases anteriores afectadas sigue pasando (56/56 suites en CTest 100% exitosas).
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos (Esquema v11 con `isHardcore`, `gameRules`, `unlockedAdvancements`, `generalStatistics`).
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase es la **Fase 14 (Desempeño masivo y optimizaciones de escala)**.

