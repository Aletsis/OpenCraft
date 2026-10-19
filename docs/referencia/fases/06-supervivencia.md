# Fase 06 — Supervivencia, movimiento y reglas de partida

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: **COMPLETADA**. Evidencias en [F06/README.md](../evidencias/F06/README.md). Dependencias de entrada: **F05**. Brechas cerradas: **B06, B15, B18**.

## Qué lograr, por qué y para qué

Transformar la construcción en una experiencia con recursos limitados, riesgo y recuperación. El jugador necesita entender por qué pierde salud y cómo continuar después de morir.

## Alcance y límites

Salud, hambre/saturación, daño ambiental, alimentación, muerte, respawn, camas, dificultad y políticas básicas de Supervivencia/Creativo. IA hostil llega en F08 y clima/luz en F07.

## Pasos de implementación

1. Implementar estadísticas y reglas de daño, invulnerabilidad temporal, regeneración y consumo de comida. Definir orden entre daño, curación y muerte dentro de un tick.

2. Agregar correr, agacharse, colisión robusta y prevención de atravesar sólidos con barrido/subpasos. Preparar nadar/oxígeno para el volumen de fluido de F07; normalizar entrada diagonal.

3. Implementar daño por caída y vacío, muerte, drops y reaparición segura. El spawn debe buscar espacio libre y suelo; no conservar la coordenada inicial fija dentro de terreno.

4. Crear cama y punto de reaparición; descanso conectado a reloj de mundo básico. La visualización de día/noche se completa en F07. Definir comportamiento cuando la cama desaparece.

5. Implementar dificultad y políticas de modo: supervivencia consume y recibe daño; creativo permite catálogo y vuelo. Espectador, Aventura y Extremo se completan en F13.

6. Añadir HUD de salud, hambre, aire cuando aplique y pantalla de muerte. Integrar guardar/reanudar sin restaurar accidentalmente estadísticas ni repetir drops.

7. Definir contrato multijugador de sueño, daño y respawn, aunque la interfaz compartida se active en F09.

## Entregables y estado al finalizar

Supervivencia local persistente, modos base separados, movimiento revisado, camas y ciclo de muerte/recuperación. Tabla de reglas y causas de daño.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Jugador con hambre baja | Comer, gastar energía y esperar regeneración con ticks controlados | Cambios coinciden con tablas fijadas; a distintos FPS se obtiene el mismo resultado. |
| Muerte con inventario conocido | Morir, guardar durante transición y reaparecer | Una sola muerte y un único conjunto de drops; no se clona inventario. |
| Cama destruida o spawn obstruido | Reaparecer después de morir | Se elige fallback seguro y se explica al jugador. |
| Borde, esquina y salto con frame lento | Reproducir movimientos a 15–144 FPS y bajo pausas | Sin atravesar paredes ni velocidad diagonal mayor; vuelo solo en modos permitidos. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: una muerte por datos de mundo no cargados. Exigir estado listo en F03 antes de simular física/daño en esa zona. Las pruebas de “primera noche” con enemigos se cierran en F08.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias.
- [x] La regresión de fases anteriores afectadas sigue pasando.
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
