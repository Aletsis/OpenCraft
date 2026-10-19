# Fase 04 — Inventario, objetos y contenedores

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: implementada. Dependencias de entrada: **F03**. Brechas: **B01, B04**. Pruebas automáticas: 18/18 pasando. Evidencias: `docs/referencia/evidencias/F04/README.md`.

## Qué lograr, por qué y para qué

Hacer que recursos y recompensas tengan existencia y cantidad. Este sistema sostiene fabricación, comercio y economía multijugador, por lo que debe impedir pérdida y duplicación.

## Alcance y límites

Definiciones de ítem independientes de bloque, stacks, metadatos, inventario, equipo, entidades de objeto, cofres y GUI táctil/ratón básica. Todavía no se entrega el catálogo completo.

## Pasos de implementación

1. Definir `ItemDefinition`, `ItemStack` con cantidad máxima por tipo, durabilidad y componentes; preparar equipos, mano secundaria y contenedores anidados con límites explícitos.

2. Reemplazar `vector<BlockID> hotbar` por referencias a slots del inventario. Implementar mover, dividir, combinar, intercambiar, recoger y soltar como transacciones validadas en simulación.

3. Agregar ítems tirados con ID, colisión, retraso de recogida y vencimiento por tick. Recoger parcialmente con inventario lleno debe dejar el sobrante en el mundo.

4. Crear bloque-entidad de cofre con inventario persistente y revisión transaccional. Un cierre de interfaz no debe borrar el objeto sostenido por el cursor.

5. Implementar panel de inventario, cantidades, tooltip y selección por toque. El layout y sus hitboxes comparten la misma geometría; añadir navegación accesible desde el inicio.

6. Persistir todos los stacks y metadatos en F02. Incorporar comandos de prueba para generar ítems, separándolos de la obtención normal en supervivencia; F05 conectará el botín minado.

## Entregables y estado al finalizar

Inventario utilizable, cofres persistentes, objetos recogibles y esquema de ítems. Galería de fixtures de stacks y prueba de conservación de recursos.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Stack de 64 y ranuras parcialmente llenas | Dividir, unir, transferir y cancelar interfaz repetidamente | Suma total constante salvo consumo/creación explícitos y ningún stack sobre su máximo. |
| Inventario casi lleno y objeto de 20 unidades | Recoger con espacio para 3 | Entran 3 y quedan 17; reiniciar conserva ambos resultados. |
| Cofre con dos comandos sobre la misma revisión | Intentar extraer el mismo stack dos veces | Solo una extracción se confirma; la otra se rechaza o recalcula. |
| Objeto en cursor y datos con durabilidad | Guardar/salir y abrir | No desaparecen ni se duplican datos; recuperación documentada del cursor. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: mantener cantidades en UI y lógica por separado. La UI solo presenta estado confirmado y peticiones. Versionar el esquema de componentes antes de añadir encantamientos.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias (`docs/referencia/evidencias/F04/README.md`).
- [x] La regresión de fases anteriores afectadas sigue pasando (18/18 pruebas automáticas en `ctest`).
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos (`test_inventory_persistence`, `test_inventory_conservation`, `test_item_entity_pickup`).
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
