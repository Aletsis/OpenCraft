# Fase 02 — Mundos persistentes y recuperación

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: COMPLETADA (15 de octubre de 2026). Evidencia: [Expediente F02](../evidencias/F02/README.md). Dependencias de entrada: **F01**. Brechas abordadas: **B02, B20**.

## Qué lograr, por qué y para qué

Conservar el esfuerzo del jugador y permitir evolucionar formatos sin perder partidas. Es requisito previo a explorar indefinidamente y a agregar objetos valiosos.

## Alcance y límites

Gestión básica de mundos, semilla/metadatos, secciones, jugador, reglas y registro de IDs; escritura recuperable, copias y migraciones. El esquema debe admitir entidades, contenedores y dimensiones que llegarán después.

## Pasos de implementación

1. Definir un formato OpenCraft con versión de esquema, versión de generador, identificador de mundo, semilla y catálogo. Separar datos globales, regiones/secciones y jugadores. Evitar serializar structs C++ crudos.

2. Implementar serializadores con límites de tamaño, checksums y manejo explícito de contenido desconocido. Los nuevos componentes tendrán un contrato de persistencia desde su incorporación.

3. Guardar snapshots coherentes a un tick; escribir a temporal y confirmar de forma recuperable según la plataforma. Implementar journal o manifiesto de transacción para cambios que involucren varios archivos.

4. Añadir autoguardado configurable, guardado al salir y cola asíncrona con contrapresión. Mostrar estado y error de disco lleno; no descartar datos sucios hasta confirmar escritura.

5. Crear menú mínimo para crear mundo, elegir semilla, listar y continuar. Separar nombres visibles de rutas internas y proteger contra apertura simultánea del mismo mundo.

6. Implementar copia de seguridad, recuperación de última versión válida y migración de una fixture de esquema anterior, conservando original. No regenerar silenciosamente chunks dañados.

## Entregables y estado al finalizar

Formato documentado, gestor de mundos accesible, guardado/carga, migrador inicial y utilidades de respaldo/recuperación. Fixtures de mundos antiguos, corruptos y con escritura interrumpida.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Mundo con 100 ediciones en chunks positivos y negativos | Guardar, cerrar proceso y abrir de nuevo | Coinciden bloques, posición, semilla y reglas; el estado confirmado tiene el mismo hash. |
| Transacción de guardado en diferentes puntos | Interrumpir el proceso antes/durante/después de confirmar | Carga el estado anterior o nuevo completo, nunca una mezcla inválida. |
| Disco sin espacio o archivo corrupto | Intentar guardar/cargar | Error comprensible, copia válida preservada y ninguna pérdida silenciosa. |
| Fixture de esquema anterior | Migrar dos veces y volver a abrir | Resultado estable, respaldo disponible y versión final correcta. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: futura duplicación de objetos por guardados parciales. Diseñar transacciones multientidad ahora y extender pruebas en F04/F09. El intervalo propuesto de autoguardado es 30 s; el máximo de progreso no confirmado perdido debe comunicarse y medirse.

## Puerta de salida
 
- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias.
- [x] La regresión de fases anteriores afectadas sigue pasando.
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
