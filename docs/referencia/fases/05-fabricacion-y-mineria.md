# Fase 05 — Minería, fabricación y herramientas

[Plan maestro](../PLAN_MAESTRO.md) · [Comparación](../COMPARACION.md)

Estado: implementada y verificada. Dependencias de entrada: **F04**. Brechas: **B01, B05**. Evidencia: [F05 README](../evidencias/F05/README.md).

## Qué lograr, por qué y para qué

Crear el primer ciclo económico completo: obtener recursos, transformarlos y mejorar herramientas. Justifica explorar y construir más allá de colocar bloques ilimitados.

## Alcance y límites

Rotura temporal, herramientas y botín, recetas 2×2/3×3, mesa, horno, combustible y libro básico. Primera progresión de madera, piedra e hierro; familias avanzadas se completan en F12/F15.

## Pasos de implementación

1. Agregar dureza, resistencia, herramienta requerida, velocidad de minado y reglas de botín a datos. Cancelar progreso al cambiar objetivo o perder alcance; mostrar grietas y retroalimentación.

2. Conectar romper/colocar con inventario: consumir solo tras colocación confirmada y generar drops según herramienta. Creativo utilizará otra política sin alterar la transacción base.

3. Crear motor de recetas con forma, sin forma, etiquetas de ingredientes, cantidades y recipientes devueltos. Validar referencias y ciclos de producción no intencionales al cargar datos.

4. Construir cuadrícula del jugador, mesa 3×3 y libro con búsqueda/faltantes; comprobar capacidad de salida antes de consumir. Cerrar una mesa conserva o devuelve ingredientes según regla documentada.

5. Implementar horno como bloque-entidad con slots de entrada/combustible/salida y progreso por tick. Definir pausa de simulación al descargar y recuperación al cargar; nunca avanzar según FPS.

6. Crear recetas y botín suficientes para tronco → tablón → mesa → pico de madera → piedra → horno → hierro → herramienta de hierro. Agregar carbón, minerales y una fuente de alimento cocinable de prueba.

7. Persistir durabilidad, combustible y proceso; documentar cómo incorporar estaciones especializadas (ahumador, alto horno, cortapiedras, herrería) sin duplicar el motor.

## Entregables y estado al finalizar

Primera cadena de producción jugable, datos de recetas y botín, herramientas con desgaste, horno persistente y guía de ampliación. Catálogo actualizado para todas las entradas añadidas.

Además: código/datos integrados, documentación de operación, migración de datos si cambia el esquema, entradas de catálogo actualizadas y evidencia reproducible de aceptación. Desde F09, cada sistema nuevo incluye réplica y pruebas con dos clientes como mínimo.

## Cómo comprobar que se logró

| Preparación | Acción | Resultado exigido |
| --- | --- | --- |
| Mundo nuevo e inventario vacío con recursos accesibles | Completar la cadena de madera a herramienta de hierro sin comandos | Cada producto tiene una ruta real de obtención y cada consumo cuadra. |
| Receta con salida bloqueada o material insuficiente | Solicitar una fabricación y luego fabricación en lote | Cero consumo si no cabe/no alcanza; sin cantidades negativas. |
| Horno a mitad de proceso | Guardar, descargar chunk y volver a cargar | Progreso y combustible siguen la política documentada sin duplicar salidas. |
| Herramienta con un uso restante | Minar y repetir petición de rotura | Una rotura válida, durabilidad agotada y un único lote de botín. |

Registrar build, semilla, comandos, dispositivo, resultados esperados/obtenidos y enlace a logs o captura. Una prueba descrita aquí es trabajo futuro; no implica que ya se haya ejecutado.

## Riesgos y decisiones técnicas

Riesgo: balancear a mano valores inconsistentes. Tablas por versión con unidades y pruebas de duración. No llenar el código de condiciones por nombre de bloque; usar capacidades y etiquetas.

## Puerta de salida

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias.
- [x] La regresión de fases anteriores afectadas sigue pasando.
- [x] Guardado/carga y límites de recursos se prueban con los nuevos datos.
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las diferencias con la referencia y el trabajo de catálogo restante están identificados.

La siguiente fase puede planificarse antes, pero no debe declararse terminada mientras le falte esta dependencia. Los mínimos de esta fase habilitan sistemas; la cobertura total de contenido se verifica en F15.
