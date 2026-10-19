# Fase 15 — Catálogo completo, certificación y lanzamiento

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: **Completada (🟢 Certificada)**. Dependencias de entrada: **F00–F14**. Brechas: **B01–B23 (Cerradas)**. Evidencias: [evidencias/F15](../evidencias/F15/README.md).

## Qué lograr, por qué y para qué

Convertir sistemas representativos en cobertura completa de la referencia y certificar una entrega estable. Terminar la primera noche o vencer un jefe no basta para afirmar paridad.

## Alcance y límites

Cierre exhaustivo del catálogo Java 1.21 acordado en F00, pruebas cruzadas, rendimiento, compatibilidad de guardados y paquetes finales. Lanzamiento se entiende como preparar entrega; publicar externamente es una acción posterior.

## Pasos de implementación

1. Reconciliar todas las filas del catálogo con registros implementados. Dividir por lotes: construcción/decoración; objetos/recetas; fauna y combate; biomas/estructuras; automatización; progresión; comandos/avances. Cada entrada tiene responsable, dependencias, fixture y fuente.

2. Completar variedades y mecánicas omitidas por los lotes iniciales: formas/orientaciones, estaciones, tintes, carteles/libros/mapas, objetos especiales, criaturas acuáticas/voladoras, eventos e incursiones, estructuras especiales, arqueología y colecciones. Volver a sus fases propietarias para añadir soporte cuando sea necesario.

3. Incluir explícitamente contenido 1.21: cámaras de pruebas, sus generadores/recompensas, Torbellino/Esqueleto del pantano equivalentes, maza y crafter, además de variantes decorativas. Sus interacciones también cuentan, no solo los modelos.

4. Crear escenarios de comparación para cada comportamiento: fuente/versión, estado inicial, acciones por tick, resultado esperado y tolerancia. Marcar diferencias visuales permitidas aparte de diferencias funcionales.

5. Ejecutar regresión combinada: inventario + muerte + desconexión; portal + guardado; pistón + cofre + descarga; clima + luz + spawn; paquete + migración. Corregir fallos en la fase propietaria y repetir solo suites afectadas más recorrido de humo.

6. Realizar prueba de ocho horas de servidor y sesiones prolongadas móviles, auditoría de memoria, recuperación de mundos y actualización desde todas las versiones OpenCraft soportadas.

7. Preparar candidato de lanzamiento con binarios, checksums, changelog, manual, requisitos, copias/migración, diferencias conocidas y dossier de evidencias. Establecer política de mantenimiento y backlog separado de versiones el juego de referencia posteriores.

## Entregables y estado al finalizar

Catálogo completamente reconciliado, dossier de pruebas, mundos de referencia, candidato instalable y declaración precisa de alcance. Si hay diferencias mecánicas abiertas se entrega como equivalencia parcial, nunca como paridad completa.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Catálogo congelado y build candidato | Comprobar cada fila y enlazar prueba ejecutada | 100 % de requisitos obligatorios aprobados; ninguna fila pendiente se excluye para inflar cobertura. |
| Mundos de versiones previas y respaldo | Actualizar, jugar, guardar y restaurar copia | Migración conserva estado; reversión se hace desde respaldo, sin prometer downgrade no soportado. |
| Servidor cuatro jugadores, granjas y circuitos | Mantener ocho horas, reconexiones y guardados periódicos | Cero corrupción/duplicación/crashes; memoria acotada y métricas dentro del presupuesto. |
| Candidato en cada plataforma declarada | Instalar y ejecutar recorridos de supervivencia, creativo y cooperación | Todos los recorridos pasan con evidencias y sin bloqueadores abiertos. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: el cierre de catálogo puede ser mayor que varias fases anteriores. Mantener entregas parciales por familias, no comprimirlo en “pulido”. Una excepción aceptada cambia la etiqueta de alcance; no convierte una diferencia real en igualdad.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias.
- [x] La regresión de fases anteriores afectadas sigue pasando.
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

