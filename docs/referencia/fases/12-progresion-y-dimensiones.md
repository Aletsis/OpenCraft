# Fase 12 — Progresión avanzada, dimensiones y jefes

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: 🟢 **Completada** ([Evidencias](../evidencias/F12/README.md)). Dependencias de entrada: **F11**. Brechas: **B15, B16, B17**.

## Qué lograr, por qué y para qué

Dar una ruta de largo plazo desde las herramientas iniciales hasta desafíos y recompensas finales, manteniendo la libertad de seguir construyendo.

## Alcance y límites

Experiencia, encantamientos, yunque/reparación, herrería avanzada, pociones/efectos, Abismo y Confín equivalentes, portales, jefes y movilidad posterior. Modelos, nombres de presentación y recursos artísticos serán propios.

## Pasos de implementación

1. Crear grafo de desbloqueos y auditar cada ingrediente desde el mundo inicial. Ningún acceso debe requerir un recurso que solo se encuentre detrás de ese mismo acceso.

2. Implementar niveles/experiencia y encantamientos con compatibilidades, costes y fuentes; yunque, reparación, desencantado y mejora de equipo según catálogo. Persistir todos los modificadores.

3. Crear soporte de pociones, ingredientes, combustible y efectos con duración/intensidad/reglas de combinación. Aplicar efectos desde combate, ambiente y consumo con orden determinista.

4. Generalizar clave de chunk a dimensión + coordenadas, cada una con generador, reloj/reglas y almacenamiento. Añadir portales con búsqueda de destino seguro y relaciones de coordenadas documentadas.

5. Implementar dimensión infernal y dimensión final con recursos, estructuras y criaturas necesarias. Conectar rutas de acceso, muerte, regreso y transferencia de entidades en red.

6. Agregar jefe final equivalente al dragón y jefe invocable equivalente a la Hidra ósea, recompensas únicas y reinvocación según reglas. Añadir baliza y vuelo planeado/élitros equivalentes como metas posteriores.

7. Completar un recorrido sin comandos desde mundo nuevo hasta victoria y regreso. Incorporar guardados en cada transición para verificar que la progresión puede reanudarse.

## Entregables y estado al finalizar

Ruta completa de progresión, tres dimensiones de la referencia, equipo avanzado, alquimia y jefes. Diagrama de dependencias de recursos y partidas de prueba antes/después de cada hito.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Mundo nuevo sin privilegios | Recorrer obtención de recursos, acceso a dimensiones, jefe y vuelta | Ruta completada con recursos obtenibles; mundo sigue jugable después del final. |
| Jugador y objeto al cruzar portal | Interrumpir conexión/guardado en la transferencia | Existe exactamente una instancia en un destino recuperable y conserva inventario. |
| Efectos y encantamientos combinados | Aplicar, expirar, morir y guardar/cargar | Daño, coste y duración coinciden con reglas y no quedan modificadores fantasma. |
| Jefe atacado por varios jugadores | Derrotar, recargar y reinvocar cuando corresponda | Recompensa/evento únicos por encuentro y estados sincronizados. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: explosión de contenido y dependencias circulares. Implementar primero un recorrido vertical mínimo y luego cubrir familias del catálogo. Las diferencias artísticas no deben ocultar diferencias mecánicas.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias.
- [x] La regresión de fases anteriores afectadas sigue pasando.
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
